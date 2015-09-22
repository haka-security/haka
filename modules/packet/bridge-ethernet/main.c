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

static REGISTER_LOG_SECTION(bridge_ethernet);

struct ethernet_packet {
	struct packet               core_packet;
	struct time                 timestamp;
	uint64                      id;
	struct packet_module_state *state;
	struct vbuffer              data;
	int                         orig;
};

struct packet_module_state {
	int                if_fd[2];
	uint64             id;
	unsigned char     *buffer; // Generic buffer for reading
};

/* Init parameters */
static int              nb_inputs = 0;
static const char      *interfaces[2]; // At most two interfaces
static int              max_size = 0;

static void cleanup()
{
}

static int ethernet_open(const char *interface)
{
	int fd;

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if (fd < 0) {
	    LOG_ERROR(bridge_ethernet, "Failed to create socket for interface '%s'. %s", interface, errno_error(errno));
	    return -1;
	}

	struct ifreq ifr;

	memset(&ifr,0,sizeof(ifr));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, IFNAMSIZ);

	int ret;

	ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));

	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "Failed to open ethernet interface (%s). %s", interface, errno_error(errno));
	    return -1;
	}

	// Get IFF flags
	ret = ioctl(fd, SIOCGIFFLAGS, &ifr);

	// Check if interface is what we asked for
	if (strcmp(interface, ifr.ifr_name)) {
	    LOG_ERROR(bridge_ethernet, "Failed to actually open ethernet interface (%s). %s", interface, errno_error(errno));
	    return -1;
	}

	// Promiscuous mode
	ifr.ifr_flags |= IFF_PROMISC;
	ret = ioctl (fd, SIOCSIFFLAGS, &ifr);

	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "Failed to set ethernet interface (%s) to promiscuous mode. %s", interface, errno_error(errno));
	    return -1;
	}

	// Retrieve MTU
	ret = ioctl(fd, SIOCGIFMTU, &ifr);

	if (ret < 0) {
	    LOG_ERROR(bridge_ethernet, "Failed to get MTU on (%s). %s", interface, errno_error(errno));
	    return -1;
	}

	if (max_size) {
		// Check if MTU matches the one from the other interface, if any
		if (max_size != ifr.ifr_mtu) {
			// It is just a warning, it may cause unexpected packet structured if sent packet is larger than output MTU
			LOG_INFO(bridge_ethernet, "Warning: MTU values don't match between interfaces.");
		}
	} else {
		max_size = ifr.ifr_mtu;
		LOG_INFO(bridge_ethernet, "Max frame size: %d bytes", max_size);
	}

	return fd;
}

static int init(struct parameters *args)
{
	const char *if_s;

	if_s = parameters_get_string(args, "devices", NULL); // Get devices list

	// Inputs
	if (if_s == NULL) {
	    LOG_ERROR(bridge_ethernet, "Please specifiy one or two ethernet interface (e.g: eth0)");
	    cleanup();
	    return 1;
	}

	char *save = NULL;
	char *in = strdup(if_s);
	while(nb_inputs < 2) {
	    char *token = strtok_r(in, ", \t", &save);
	    if (token == NULL) break;
	    interfaces[nb_inputs++] = (const char*)strdup(token);
	    LOG_INFO(bridge_ethernet, "Using ethernet interface %s", token);
	    free(in);
	    in = NULL;
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
	free(state);
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;
	int i;

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

	for (i=0; i<nb_inputs; ++i) {
	    int fd = ethernet_open(interfaces[i]);
	    if (fd < 0) {
	        cleanup_state(state);
	        return NULL;
	    }
	    state->if_fd[i] = fd;
	}

	state->id = 0;

	state->buffer = (unsigned char *)malloc(max_size);

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
	    if (fd < 0) {
	        LOG_ERROR(bridge_ethernet, "Invalid descriptor.");
	        return 1;
	    }
	    else {
	        FD_SET(fd, &read_set);
	        if (fd > max_fd) max_fd = fd;
	    }
	}
	FD_SET(engine_thread_interrupt_fd(), &read_set);
	if (engine_thread_interrupt_fd() > max_fd) max_fd = engine_thread_interrupt_fd();

	ret = select(max_fd+1, &read_set, NULL, NULL, NULL);
	if (ret < 0) {
		if (errno == EINTR) {
			return 0;
		}
		else {
			LOG_ERROR(bridge_ethernet, "%s", errno_error(errno));
			return 1;
		}
	} else {
		// Check for interrupt
		if (FD_ISSET(engine_thread_interrupt_fd(), &read_set))
			return 0;

		// Check inputs
		for(i = 0; i < nb_inputs; i++) {
			if (FD_ISSET(state->if_fd[i], &read_set)) {
				int length;

				// Read ETH packet
				length = recvfrom(state->if_fd[i], &state->buffer[0], max_size, 0, NULL, NULL);
				if (length < 0) {
					LOG_ERROR(bridge_ethernet, "%s", errno_error(errno));
					return 1;
				}

				// Create packet
				struct ethernet_packet *packet = malloc(sizeof(struct ethernet_packet));
				if (!packet) {
					return ENOMEM;
				}

				memset(packet, 0, sizeof(struct ethernet_packet));

				time_gettimestamp(&packet->timestamp);

				if (!vbuffer_create_from(&packet->data, (char *)state->buffer, length)) {
					free(packet);
					return ENOMEM;
				}

				packet->state     = state;
				packet->orig      = i; // Remember where we came from
				packet->id        = state->id++;

				*pkt = (struct packet *)packet;
				return 0;
			}
		}
	}

	return 0;
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	struct ethernet_packet *pkt = (struct ethernet_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data)) {
	    // If packet accepted and we need to forward to the other port
	    if (nb_inputs > 1 && result == FILTER_ACCEPT) {
	        const uint8 *data;
	        size_t len;
	        int out;

	        data = vbuffer_flatten(&pkt->data, &len);
	        if (!data) {
	            assert(check_error());
	            vbuffer_clear(&pkt->data);
	            return;
	        }

	        if (pkt->orig == 0) out = pkt->state->if_fd[1];
	        else                out = pkt->state->if_fd[0];

	        if (sendto(out, data, len, 0, NULL, 0) < 0) {
	            LOG_ERROR(bridge_ethernet, "%s", errno_error(errno));
	        }
	    }

	    vbuffer_clear(&pkt->data);
	}
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	return "ethernet"; // ?
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	return ((struct ethernet_packet*)orig_pkt)->id;
}

static void packet_do_release(struct packet *orig_pkt)
{
	struct ethernet_packet *pkt = (struct ethernet_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data)) {
	    packet_verdict(orig_pkt, FILTER_DROP);
	}

	vbuffer_release(&pkt->core_packet.payload);
	vbuffer_release(&pkt->data);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct ethernet_packet *pkt = (struct ethernet_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data)) {
	    return STATUS_NORMAL;
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

	packet->state = state;
	packet->id = state->id++;
	time_gettimestamp(&packet->timestamp);

	if (!vbuffer_create_new(&packet->data, size, true)) {
	    assert(check_error());
	    free(packet);
	    return NULL;
	}

	return (struct packet *)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	if (nb_inputs < 2) {
	    error("sending is not supported in pass-through");
	    return false;
	}
	return false;
}

static size_t get_mtu(struct packet *pkt)
{
	return max_size;
}

static const struct time *get_timestamp(struct packet *orig_pkt)
{
	struct ethernet_packet *pkt = (struct ethernet_packet*)orig_pkt;
	return &pkt->timestamp;
}

static bool is_realtime()
{
	return true;
}

struct packet_module HAKA_MODULE = {
	module: {
	    type:        MODULE_PACKET,
	    name:        "Ethernet Module",
	    description: "Ethernet packet module",
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
