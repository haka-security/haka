/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/parameters.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/engine.h>

// Ethernet header is not included in MTU size. 22 = Max that VLan can accept
#define ETHER_HEADERSIZE 22

static REGISTER_LOG_SECTION(bridge_ethernet);

struct ethernet_packet {
	struct packet               core_packet;
	struct time                 timestamp;
	uint64                      id;
	struct packet_module_state *state;
	int                         orig;
};

struct packet_module_state {
	int                if_fd[2];
	uint64             id;
	unsigned char     *buffer; // Generic buffer for reading
	int                mtu;
	int                bypass; // If 1 everything received is immediately sent to other end. Implies two interfaces
};

/* Init parameters */
static int       nb_inputs = 0;
static char     *interfaces[2] = { NULL, NULL }; // At most two interfaces

static void cleanup()
{
	free(interfaces[0]);
	free(interfaces[1]);
}

static int ethernet_open(const char *interface, int *mtu)
{
	int ret;
	int fd;
	struct ifreq ifr;
	struct sockaddr_ll sock_address;

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
	    LOG_ERROR(bridge_ethernet, "failed to create socket for interface %s: %s", interface, errno_error(errno));
	    return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, IFNAMSIZ);
	ret = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "failed to retrieve interface index for %s: %s", interface, errno_error(errno));
	    goto bailout;
	}

	memset(&sock_address, 0, sizeof(sock_address));
	sock_address.sll_family = AF_PACKET;
	sock_address.sll_protocol = htons(ETH_P_ALL);
	sock_address.sll_ifindex = ifr.ifr_ifindex;
	ret = bind(fd, (struct sockaddr *)&sock_address, sizeof(sock_address));
	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "failed to bind to interface %s: %s", interface, errno_error(errno));
	    goto bailout;
	}

	// Get IFF flags
	ret = ioctl(fd, SIOCGIFFLAGS, &ifr);

	// Promiscuous mode
	ifr.ifr_flags |= IFF_PROMISC;
	ret = ioctl(fd, SIOCSIFFLAGS, &ifr);
	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "failed to set ethernet interface %s to promiscuous mode: %s", interface, errno_error(errno));
	    goto bailout;
	}

	// Retrieve MTU
	ret = ioctl(fd, SIOCGIFMTU, &ifr);
	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "failed to get MTU on %s: %s", interface, errno_error(errno));
	    goto bailout;
	}

	assert(mtu);
	*mtu = ifr.ifr_mtu + ETHER_HEADERSIZE;

	return fd;

bailout:
	close(fd);
	return -1;
}

static void ethernet_close(struct packet_module_state *state, int i)
{
	struct ifreq ifr;
	int ret;

	assert(i >= 0 && i < 2);

	if (state->if_fd[i] < 0) return;

	// Get IFF flags
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_ifrn.ifrn_name, interfaces[i], IFNAMSIZ);
	ret = ioctl(state->if_fd[i], SIOCGIFFLAGS, &ifr);
	if (ret < 0) {
	    LOG_WARNING(bridge_ethernet, "failed to get IFF Flags on %s: %s", interfaces[i], errno_error(errno));
	    goto end;
	}

	// Reset promiscuous mode
	ifr.ifr_flags &= ~IFF_PROMISC;
	ret = ioctl(state->if_fd[i], SIOCSIFFLAGS, &ifr);
	if (ret < 0) {
	    LOG_WARNING(bridge_ethernet, "failed to reset promiscuous mode on %s: %s", interfaces[i], errno_error(errno));
	    goto end;
	}

end:
	close(state->if_fd[i]);
	state->if_fd[i] = -1;
}

static int init(struct parameters *args)
{
	const char *if_s;
	char *save;
	char *in;

	assert(args);

	if_s = parameters_get_string(args, "interfaces", NULL); // Get devices list
	if (if_s == NULL) {
		LOG_ERROR(bridge_ethernet, "please specify 'interfaces' parameter in configuration file");
		cleanup();
		return 1;
	}

	in = (char *)if_s;
	while (nb_inputs < 2) {
		char *token = strtok_r(in, ", \t", &save);
		if (token == NULL) break;
		interfaces[nb_inputs++] = strdup(token);
		LOG_INFO(bridge_ethernet, "using ethernet interface %s", token);
		in = NULL;
	}

	if (nb_inputs == 0) {
		LOG_ERROR(bridge_ethernet, "please specifiy one or two ethernet interfaces (e.g: eth0)");
		cleanup();
		return 1;
	}

	return 0;
}

static bool multi_threaded()
{
	return false;
}

static bool pass_through()
{
	return nb_inputs < 2;
}

static void cleanup_state(struct packet_module_state *state)
{
	free(state->buffer);
	ethernet_close(state, 0);
	ethernet_close(state, 1);
	free(state);
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;
	int i;
	int mtu[] = { 0, 0 };

	assert(nb_inputs > 0);

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		error("memory error");
		return NULL;
	}

	memset(state, 0, sizeof(struct packet_module_state));

	// Invalid descriptors at init
	state->if_fd[0] = -1;
	state->if_fd[1] = -1;

	for (i=0; i < nb_inputs; ++i) {
		int fd = ethernet_open(interfaces[i], &mtu[i]);
		if (fd < 0) {
			cleanup_state(state);
			return NULL;
		}
		state->if_fd[i] = fd;
	}

	state->id = 0;
	state->mtu = MAX(mtu[0], mtu[1]);
	// Check if MTU matches the one from the other interface, if any
	if (nb_inputs == 2 && mtu[0] != mtu[1]) {
		// It is just a warning, it may cause unexpected packet structured if sent packet is larger than output MTU
		LOG_WARNING(bridge_ethernet, "MTU values don't match between interfaces: %d != %d", mtu[0], mtu[1]);
	}
	LOG_INFO(bridge_ethernet, "max frame size: %d bytes", state->mtu);

	state->buffer = (unsigned char *)malloc(state->mtu);
	if (!state->buffer) {
		cleanup_state(state);
		error("memory error");
		return NULL;
	}

	return state;
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	int i;
	int ret;
	fd_set read_set;
	int max_fd = -1;

	// Read packet
	FD_ZERO(&read_set);

	for (i=0; i < nb_inputs; ++i) {
		const int fd = state->if_fd[i];
		assert(fd >= 0);
		FD_SET(fd, &read_set);
		if (fd > max_fd) max_fd = fd;
	}
	FD_SET(engine_thread_interrupt_fd(), &read_set);
	if (engine_thread_interrupt_fd() > max_fd) max_fd = engine_thread_interrupt_fd();

	ret = select(max_fd+1, &read_set, NULL, NULL, NULL);
	if (ret < 0) {
		if (errno == EINTR) {
			return 0;
		} else {
			LOG_ERROR(bridge_ethernet, "select: %s", errno_error(errno));
			return 1;
		}
	}

	// Check for interrupt
	if (FD_ISSET(engine_thread_interrupt_fd(), &read_set))
		return 0;

	// Check bypass first
	if (state->bypass) {
		assert(nb_inputs == 2);
		for (i = 0; i < nb_inputs; i++) {
			if (FD_ISSET(state->if_fd[i], &read_set)) {
				int length;
				// Read from one IF
				length = recvfrom(state->if_fd[i], state->buffer, state->mtu, 0, NULL, NULL);
				if (length < 0) {
					LOG_ERROR(bridge_ethernet, "recvfrom: %s", errno_error(errno));
					return 1;
				}
				// Forward to the other (only 2)
				if (sendto(state->if_fd[i^1], state->buffer, length, 0, NULL, 0) < 0) {
					LOG_ERROR(bridge_ethernet, "sendto: %s", errno_error(errno));
					return 1;
				}
			}
		}
	} else {
		// Check inputs
		for (i = 0; i < nb_inputs; i++) {
			if (FD_ISSET(state->if_fd[i], &read_set)) {
				int length;

				// Read ETH packet
				length = recvfrom(state->if_fd[i], state->buffer, state->mtu, 0, NULL, NULL);
				if (length < 0) {
					LOG_ERROR(bridge_ethernet, "recvfrom: %s", errno_error(errno));
					return 1;
				}

				// Create packet
				struct ethernet_packet *packet = malloc(sizeof(struct ethernet_packet));
				if (!packet) {
					return ENOMEM;
				}

				memset(packet, 0, sizeof(struct ethernet_packet));

				time_gettimestamp(&packet->timestamp);

				if (!vbuffer_create_from(&packet->core_packet.payload, (char *)state->buffer, length)) {
					free(packet);
					return ENOMEM;
				}

				packet->state = state;
				packet->orig  = i; // Remember where we came from
				packet->id    = state->id++;

				*pkt = (struct packet *)packet;
				return 0;
			}
		}
	}

	return 0;
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	struct ethernet_packet *pkt = (struct ethernet_packet *)orig_pkt;

	if (vbuffer_isvalid(&pkt->core_packet.payload)) {
		// If packet accepted and we need to forward to the other port
		if (nb_inputs > 1 && result == FILTER_ACCEPT) {
			const uint8 *data;
			size_t len;
			int out;

			data = vbuffer_flatten(&pkt->core_packet.payload, &len);
			if (!data) {
				assert(check_error());
				vbuffer_clear(&pkt->core_packet.payload);
				return;
			}

			assert(pkt->orig != -1);

			if (pkt->orig == 0) out = pkt->state->if_fd[1];
			else                out = pkt->state->if_fd[0];

			if (sendto(out, data, len, 0, NULL, 0) < 0) {
				LOG_ERROR(bridge_ethernet, "sendto: %s", errno_error(errno));
			}
		}

		vbuffer_clear(&pkt->core_packet.payload);
	}
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	return "ethernet";
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	return ((struct ethernet_packet *)orig_pkt)->id;
}

static void packet_do_release(struct packet *orig_pkt)
{
	struct ethernet_packet *pkt = (struct ethernet_packet *)orig_pkt;

	if (vbuffer_isvalid(&pkt->core_packet.payload)) {
		packet_verdict(orig_pkt, FILTER_DROP);
	}

	vbuffer_release(&pkt->core_packet.payload);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct ethernet_packet *pkt = (struct ethernet_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->core_packet.payload)) {
		if (pkt->orig == -1) return STATUS_FORGED;
		else                 return STATUS_NORMAL;
	} else {
		return STATUS_SENT;
	}
}

static struct packet *new_packet(struct packet_module_state *state, size_t size)
{
	struct ethernet_packet *packet = malloc(sizeof(struct ethernet_packet));
	if (!packet) {
		error("Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct ethernet_packet));

	packet->orig  = -1; // Unknown origin
	packet->state = state;
	packet->id = -1;
	time_gettimestamp(&packet->timestamp);

	if (!vbuffer_create_new(&packet->core_packet.payload, size, true)) {
		assert(check_error());
		free(packet);
		return NULL;
	}

	return (struct packet *)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	struct ethernet_packet *pkt = (struct ethernet_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->core_packet.payload)) {
		// We send data to both ends. Currently we have no mean from lua
		// level to specify where to send the forged packet.
		const uint8 *data;
		size_t len;
		int ret;

		data = vbuffer_flatten(&pkt->core_packet.payload, &len);
		if (!data) {
			assert(check_error());
			vbuffer_clear(&pkt->core_packet.payload);
			return false;
		}

		ret = sendto(pkt->state->if_fd[0], data, len, 0, NULL, 0);
		if (ret < 0) {
			LOG_ERROR(bridge_ethernet, "sendto %s: %s", interfaces[0], errno_error(errno));
		}

		if (nb_inputs > 1) {
			if (sendto(pkt->state->if_fd[1], data, len, 0, NULL, 0) < 0) {
				LOG_ERROR(bridge_ethernet, "sendto: %s", errno_error(errno));
				ret = -1;
			}
		}

		if (ret < 0) {
			return false;
		}

		vbuffer_clear(&pkt->core_packet.payload);
		return true;
	}
	return false;
}

static size_t get_mtu(struct packet *pkt)
{
	return ((struct ethernet_packet *)pkt)->state->mtu;
}

static const struct time *get_timestamp(struct packet *pkt)
{
	return &((struct ethernet_packet *)pkt)->timestamp;
}

static bool is_realtime()
{
	return true;
}

struct packet_module HAKA_MODULE = {
	module: {
	    type:        MODULE_PACKET,
	    name:        "Ethernet Module",
	    description: "Ethernet bridge packet module",
	    api_version: HAKA_API_VERSION,
	    init:        init,
	    cleanup:     cleanup
	},
	multi_threaded:  multi_threaded,
	pass_through:    pass_through,
	is_realtime:     is_realtime,
	init_state:      init_state,
	cleanup_state:   cleanup_state,
	receive:         packet_do_receive,
	verdict:         packet_verdict,
	get_id:          packet_get_id,
	get_dissector:   packet_get_dissector,
	release_packet:  packet_do_release,
	packet_getstate: packet_getstate,
	new_packet:      new_packet,
	send_packet:     send_packet,
	get_mtu:         get_mtu,
	get_timestamp:   get_timestamp
};
