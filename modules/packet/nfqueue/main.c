/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/system.h>
#include <haka/engine.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "iptables.h"
#include <pcap.h>
#include "config.h"


#define PACKET_BUFFER_SIZE	0xffff
/* Should be enough to receive the packet and the extra headers */
#define PACKET_RECV_SIZE	70000

#define MAX_INTERFACE 8


REGISTER_LOG_SECTION(nfqueue);

struct pcap_dump {
	pcap_t        *pd;
	pcap_dumper_t *pf;
	mutex_t        mutex;
};

struct pcap_sinks {
	struct pcap_dump   in;
	struct pcap_dump   out;
};

struct packet_module_state {
	struct nfq_handle          *handle;
	struct nfq_q_handle        *queue;
	int                         fd;
	int                         send_fd;
	int                         send_mark_fd;

	struct nfqueue_packet      *current_packet; /* Packet allocated by nfq callback */
	int                         error;
	char                        receive_buffer[PACKET_RECV_SIZE];
};

static struct pcap_sinks       *pcap = NULL;

struct nfqueue_packet {
	struct packet               core_packet;
	struct packet_module_state *state;
	int                         id; /* nfq identifier */
	struct time                 timestamp;
};

bool use_multithreading = true;
size_t nfqueue_len = 1024;

/* Iptables rules to add (iptables-restore format) */
static const char iptables_config_template_begin[] =
"*raw\n"
":" HAKA_TARGET_PRE " - [0:0]\n"
":" HAKA_TARGET_OUT " - [0:0]\n"
;

static const char iptables_config_install_begin[] =
":PREROUTING ACCEPT [0:0]\n"
":OUTPUT ACCEPT [0:0]\n"
;

static const char iptables_config_template_mt_iface[] =
"-A " HAKA_TARGET_PRE " -i %s -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_PRE " -i %s -j NFQUEUE --queue-balance 0:%i\n"
"-A " HAKA_TARGET_OUT " -o %s -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_OUT " -o %s -j MARK --set-mark 0xffff\n"
"-A " HAKA_TARGET_OUT " -o %s -j NFQUEUE --queue-balance 0:%i\n"
;

static const char iptables_config_template_iface[] =
"-A " HAKA_TARGET_PRE " -i %s -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_PRE " -i %s -j NFQUEUE\n"
"-A " HAKA_TARGET_OUT " -o %s -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_OUT " -o %s -j MARK --set-mark 0xffff\n"
"-A " HAKA_TARGET_OUT " -o %s -j NFQUEUE\n"
;

static const char iptables_config_template_mt_all[] =
"-A " HAKA_TARGET_PRE " -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_PRE " -j NFQUEUE --queue-balance 0:%i\n"
"-A " HAKA_TARGET_OUT " -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_OUT " -j MARK --set-mark 0xffff\n"
"-A " HAKA_TARGET_OUT " -j NFQUEUE --queue-balance 0:%i\n"
;

static const char iptables_config_template_all[] =
"-A " HAKA_TARGET_PRE " -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_PRE " -j NFQUEUE\n"
"-A " HAKA_TARGET_OUT " -m mark --mark 0xffff -j ACCEPT\n"
"-A " HAKA_TARGET_OUT " -j MARK --set-mark 0xffff\n"
"-A " HAKA_TARGET_OUT " -j NFQUEUE\n"
;

static const char iptables_config_install_end[] =
"-A PREROUTING -j " HAKA_TARGET_PRE "\n"
"-A OUTPUT -j " HAKA_TARGET_OUT "\n"
;
static const char iptables_config_template_end[] =
"COMMIT\n"
;

static int iptables_config_build(char *output, size_t outsize, char **ifaces, int threads, bool install)
{
	int size, total_size = 0;

	size = snprintf(output, outsize, iptables_config_template_begin);
	if (!size) return -1;
	if (output) { output += size; outsize-=size; }
	total_size += size;

	if (install) {
		size = snprintf(output, outsize, iptables_config_install_begin);
		if (!size) return -1;
		if (output) { output += size; outsize-=size; }
		total_size += size;
	}

	if (ifaces) {
		char **iface = ifaces;
		while (*iface) {
			if (threads > 1) {
				size = snprintf(output, outsize, iptables_config_template_mt_iface, *iface, *iface,
						threads-1, *iface, *iface, *iface, threads-1);
			}
			else {
				size = snprintf(output, outsize, iptables_config_template_iface, *iface, *iface,
						*iface, *iface, *iface);
			}

			if (!size) return -1;
			if (output) { output += size; outsize-=size; }
			total_size += size;

			++iface;
		}
	}
	else {
		if (threads > 1) {
			size = snprintf(output, outsize, iptables_config_template_mt_all, threads-1, threads-1);
		}
		else {
			size = snprintf(output, outsize, iptables_config_template_all);
		}

		if (!size) return -1;
		if (output) { output += size; outsize-=size; }
		total_size += size;
	}

	if (install) {
		size = snprintf(output, outsize, iptables_config_install_end);
		if (!size) return -1;
		if (output) { output += size; outsize-=size; }
		total_size += size;
	}

	size = snprintf(output, outsize, iptables_config_template_end);
	if (!size) return -1;
	if (output) { output += size; outsize-=size; }
	total_size += size;

	return total_size;
}

static char *iptables_config(char **ifaces, int threads, bool install)
{
	char *new_iptables_config;

	const int size = iptables_config_build(NULL, 0, ifaces, threads, install);
	if (size <= 0) {
		return NULL;
	}

	new_iptables_config = malloc(sizeof(char) * (size+1));
	if (!new_iptables_config) {
		return NULL;
	}

	iptables_config_build(new_iptables_config, size+1, ifaces, threads, install);
	return new_iptables_config;
}

/* Iptables raw table current configuration */
static char *iptables_saved = NULL;
static bool iptables_save_need_flush = true;


static int packet_callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		struct nfq_data *nfad, void *data)
{
	struct packet_module_state *state = data;
	struct nfqnl_msg_packet_hdr* packet_hdr;

#if NFQ_GET_PAYLOAD_UNSIGNED_CHAR
	unsigned char *packet_data;
#else
	char *packet_data;
#endif

	int packet_len;

	state->error = 0;

	packet_hdr = nfq_get_msg_packet_hdr(nfad);
	if (!packet_hdr) {
		LOG_ERROR(nfqueue_section, "unable to get packet header");
		return 0;
	}

	packet_len = nfq_get_payload(nfad, &packet_data);
	if (packet_len > PACKET_BUFFER_SIZE) {
		LOG_WARNING(nfqueue_section, "received packet is too large");
		return 0;
	}
	else if (packet_len < 0) {
		LOG_ERROR(nfqueue_section, "unable to get packet payload");
		return 0;
	}

	state->current_packet = malloc(sizeof(struct nfqueue_packet));
	if (!state->current_packet) {
		state->error = ENOMEM;
		return 0;
	}

	memset(state->current_packet, 0, sizeof(struct nfqueue_packet));

	if (!vbuffer_create_from(&state->current_packet->core_packet.payload,
	    (char *)packet_data, packet_len)) {
		free(state->current_packet);
		state->error = ENOMEM;
		return 0;
	}

	time_gettimestamp(&state->current_packet->timestamp);
	state->current_packet->id = ntohl(packet_hdr->packet_id);

	return 0;
}

static void cleanup_state(struct packet_module_state *state)
{
	if (state->queue)
		nfq_destroy_queue(state->queue);
	if (state->handle)
		nfq_close(state->handle);

	state->queue = NULL;
	state->handle = NULL;

	if (state->send_fd >= 0) close(state->send_fd);
	if (state->send_mark_fd >= 0) close(state->send_mark_fd);

	free(state);
}

static int open_send_socket(bool mark)
{
	int fd;
	int one = 1;

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (fd < 0) {
		LOG_ERROR(nfqueue_section, "cannot open send socket: %s", errno_error(errno));
		return -1;
	}

	if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
		close(fd);
		LOG_ERROR(nfqueue_section, "cannot setup send socket: %s", errno_error(errno));
		return -1;
	}

	if (mark) {
		one = 0xffff;
		if (setsockopt(fd, SOL_SOCKET, SO_MARK, &one, sizeof(one)) < 0) {
			close(fd);
			LOG_ERROR(nfqueue_section, "cannot setup send socket: %s", errno_error(errno));
			return -1;
		}
	}

	return fd;
}

static bool socket_send_packet(int fd, const void *pkt, size_t size)
{
	struct sockaddr_in sin;
	const struct iphdr *iphdr = pkt;

	sin.sin_family = AF_INET;
	sin.sin_port = iphdr->protocol;
	sin.sin_addr.s_addr = iphdr->daddr;

	if (sendto(fd, pkt, size, 0, (struct sockaddr*)&sin, sizeof(sin)) == (size_t)-1) {
		error("send failed: %s", errno_error(errno));
		return false;
	}

	return true;
}

static struct packet_module_state *init_state(int thread_id)
{
	static const u_int16_t proto_family[] = { AF_INET, AF_INET6 };
	int i;
	struct packet_module_state *state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		return NULL;
	}

	state->handle = NULL;
	state->queue = NULL;
	state->send_fd = -1;
	state->send_mark_fd = -1;

	/* Setup nfqueue connection */
	state->handle = nfq_open();
	if (!state->handle) {
		LOG_ERROR(nfqueue_section, "unable to open nfqueue handle");
		cleanup_state(state);
		return NULL;
	}

	for (i=0; i<sizeof(proto_family)/sizeof(proto_family[0]); ++i) {
		if (nfq_unbind_pf(state->handle, proto_family[i]) < 0) {
			LOG_ERROR(nfqueue_section, "cannot unbind queue");
			cleanup_state(state);
			return NULL;
		}

		if (nfq_bind_pf(state->handle, proto_family[i]) < 0) {
			LOG_ERROR(nfqueue_section, "cannot bind queue");
			cleanup_state(state);
			return NULL;
		}

		state->send_fd = open_send_socket(false);
		if (state->send_fd < 0) {
			cleanup_state(state);
			return NULL;
		}

		state->send_mark_fd = open_send_socket(true);
		if (state->send_fd < 0) {
			cleanup_state(state);
			return NULL;
		}
	}

	state->queue = nfq_create_queue(state->handle, thread_id,
			&packet_callback, state);
	if (!state->queue) {
		LOG_ERROR(nfqueue_section, "cannot create queue");
		cleanup_state(state);
		return NULL;
	}

	if (nfq_set_mode(state->queue, NFQNL_COPY_PACKET,
			PACKET_BUFFER_SIZE) < 0) {
		LOG_ERROR(nfqueue_section, "cannot set mode to copy packet");
		cleanup_state(state);
		return NULL;
	}

	state->fd = nfq_fd(state->handle);

	/* Change nfq queue len and netfilter receive size */
	if (nfq_set_queue_maxlen(state->queue, nfqueue_len) < 0) {
		LOG_WARNING(nfqueue_section, "cannot change netfilter queue len");
	}

	nfnl_rcvbufsiz(nfq_nfnlh(state->handle), nfqueue_len * 1500);

	return state;
}

static int open_pcap(struct pcap_dump *pcap, const char *file)
{
	if (file) {
		pcap->pd = pcap_open_dead(DLT_IPV4, PACKET_RECV_SIZE);
		if (!pcap->pd) {
			LOG_ERROR(nfqueue_section, "cannot setup pcap sink");
			return 1;
		}

		pcap->pf = pcap_dump_open(pcap->pd, file);
		if (!pcap->pf) {
			LOG_ERROR(nfqueue_section, "cannot setup pcap sink");
			return 1;
		}
	}

	mutex_init(&pcap->mutex, false);

	return 0;
}

static void close_pcap(struct pcap_dump *pcap)
{
	if (pcap->pf) pcap_dump_close(pcap->pf);
	if (pcap->pd) pcap_close(pcap->pd);
	mutex_destroy(&pcap->mutex);
}

static void restore_iptables()
{
	if (iptables_saved) {
		if (apply_iptables("raw", iptables_saved, !iptables_save_need_flush) != 0) {
			LOG_ERROR(nfqueue_section, "cannot restore iptables rules");
		}
	}
}

static void cleanup()
{
	if (iptables_saved) {
		restore_iptables();
		free(iptables_saved);
	}

	if (pcap) {
		close_pcap(&pcap->in);
		close_pcap(&pcap->out);
		free(pcap);
		pcap = NULL;
	}
}

static bool is_iface_valid(struct ifaddrs *ifa, const char *name)
{
	for (; ifa; ifa = ifa->ifa_next) {
		if (strcmp(name, ifa->ifa_name) == 0) return true;
	}
	return false;
}

static int init(struct parameters *args)
{
	char *new_iptables_config = NULL;
	char *interfaces_buf = NULL;
	char **ifaces = NULL;
	int thread_count = thread_get_packet_capture_cpu_count();
	const char *file_in = NULL;
	const char *file_out = NULL;
	int count;
	bool dump = false;
	bool install = true;

	install = parameters_get_boolean(args, "enable_iptables", true);

	/* Setup iptables rules */
	iptables_save_need_flush = install;
	if (save_iptables("raw", &iptables_saved, install)) {
		LOG_ERROR(nfqueue_section, "cannot save iptables rules");
		cleanup();
		return 1;
	}

	system_register_fatal_cleanup(&restore_iptables);

	{
		const char *iter;
		const char *interfaces = parameters_get_string(args, "interfaces", NULL);
		if (!interfaces || strlen(interfaces) == 0) {
			LOG_ERROR(nfqueue_section, "no interfaces selected");
			cleanup();
			return 1;
		}

		for (count = 0, iter = interfaces; *iter; ++iter) {
			if (*iter == ',')
				++count;
		}

		interfaces_buf = strdup(interfaces);
		if (!interfaces_buf) {
			error("memory error");
			cleanup();
			return 1;
		}

		++count;
	}

	ifaces = malloc((sizeof(char *) * (count + 1)));
	if (!ifaces) {
		LOG_ERROR(nfqueue_section, "memory error");
		free(interfaces_buf);
		cleanup();
		return 1;
	}

	LOG_INFO(nfqueue_section, "installing iptables rules for device(s) %s", interfaces_buf);

	{
		int index = 0;
		char *str, *ptr = NULL;
		struct ifaddrs *ifa;

		if (getifaddrs(&ifa)) {
			LOG_ERROR(nfqueue_section, "%s", errno_error(errno));
			free(interfaces_buf);
			cleanup();
			return 1;
		}

		for (index = 0, str = interfaces_buf; index < count; index++, str = NULL) {
			char *token = strtok_r(str, ",", &ptr);
			assert(token != NULL);

			if (!is_iface_valid(ifa, token)) {
				LOG_ERROR(nfqueue_section, "'%s' is not a valid network interface", token);
				free(interfaces_buf);
				cleanup();
				return 1;
			}

			ifaces[index] = token;
		}
		ifaces[index] = NULL;

		freeifaddrs(ifa);
	}

	if (!install) {
		LOG_WARNING(nfqueue_section, "iptables setup rely on user rules");
	}

	new_iptables_config = iptables_config(ifaces, thread_count, install);
	if (!new_iptables_config) {
		LOG_ERROR(nfqueue_section, "cannot generate iptables rules");
		free(ifaces);
		free(interfaces_buf);
		cleanup();
		return 1;
	}
	free(ifaces);
	ifaces = NULL;
	free(interfaces_buf);
	interfaces_buf = NULL;

	if (apply_iptables("raw", new_iptables_config, !install)) {
		LOG_ERROR(nfqueue_section, "cannot setup iptables rules");
		free(new_iptables_config);
		cleanup();
		return 1;
	}

	free(new_iptables_config);

	/* Setup pcap dump */
	dump = parameters_get_boolean(args, "dump", false);
	if (dump) {
		file_in = parameters_get_string(args, "dump_input", NULL);
		file_out = parameters_get_string(args, "dump_output", NULL);
		if (!(file_in || file_out)) {
			LOG_WARNING(nfqueue_section, "no dump pcap files specified");
		}
		else {
			pcap = malloc(sizeof(struct pcap_sinks));
			if (!pcap) {
				LOG_ERROR(nfqueue_section, "memory error");
				cleanup();
				return 1;
			}
			memset(pcap, 0, sizeof(struct pcap_sinks));

			if (file_in) {
				open_pcap(&pcap->in, file_in);
				LOG_INFO(nfqueue_section, "dumping received packets into '%s'", file_in);
			}
			if (file_out) {
				open_pcap(&pcap->out, file_out);
				LOG_INFO(nfqueue_section, "dumping emitted packets into '%s'", file_out);
			}
		}
	}

	return 0;
}

static bool multi_threaded()
{
	return use_multithreading;
}

static bool pass_through()
{
	return false;
}

static void dump_pcap(struct pcap_dump *pcap, struct nfqueue_packet *pkt,
		const uint8 *data, size_t len)
{
	if (pcap->pf) {
		struct pcap_pkthdr hdr;

		mutex_lock(&pcap->mutex);

		hdr.caplen = hdr.len = len;
		hdr.ts.tv_sec = pkt->timestamp.secs;
		hdr.ts.tv_usec = pkt->timestamp.nsecs / 1000;
		pcap_dump((u_char *)pcap->pf, &hdr, data);

		mutex_unlock(&pcap->mutex);
	}
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	int rv;
	fd_set read_set;
	int max_fd = -1;
	const int interrupt_fd = engine_thread_interrupt_fd();

	FD_ZERO(&read_set);

	FD_SET(state->fd, &read_set);
	max_fd = state->fd;

	FD_SET(interrupt_fd, &read_set);
	if (interrupt_fd > max_fd) max_fd = interrupt_fd;

	rv = select(max_fd+1, &read_set, NULL, NULL, NULL);
	if (rv <= 0) {
		if (rv == -1 && errno != EINTR) {
			LOG_ERROR(nfqueue_section, "packet reception failed, %s", errno_error(errno));
		}
		return 0;
	}

	if (FD_ISSET(state->fd, &read_set)) {
		rv = recv(state->fd, state->receive_buffer, sizeof(state->receive_buffer), 0);
		if (rv < 0) {
			if (errno != EINTR) {
				LOG_ERROR(nfqueue_section, "packet reception failed, %s", errno_error(errno));
			}
			return 0;
		}

		if (nfq_handle_packet(state->handle, state->receive_buffer, rv) == 0) {
			if (state->current_packet) {
				state->current_packet->state = state;
				*pkt = (struct packet*)state->current_packet;

				if (pcap) {
					const uint8 *data;
					size_t len;
					assert(vbuffer_isflat(&(*pkt)->payload));
					data = vbuffer_flatten(&(*pkt)->payload, &len);
					assert(data);

					dump_pcap(&pcap->in, state->current_packet, data, len);
				}

				state->current_packet = NULL;
				return 0;
			}
			else {
				return state->error;
			}
		}
		else {
			LOG_ERROR(nfqueue_section, "packet processing failed");
			return 0;
		}
	}

	/* Interruption */
	return 0;
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	int ret;
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->core_packet.payload)) {
		const uint8 *data = NULL;
		size_t len;

		if (result == FILTER_ACCEPT) {
			data = vbuffer_flatten(&pkt->core_packet.payload, &len);
			if (!data) {
				assert(check_error());
				vbuffer_clear(&pkt->core_packet.payload);
				return;
			}
		}

		if (pkt->id == -1) {
			if (result == FILTER_ACCEPT) {
				ret = socket_send_packet(pkt->state->send_mark_fd, data, len);
			}
			else {
				ret = 0;
			}
		}
		else {
			/* Convert verdict to netfilter */
			int verdict;
			switch (result) {
			case FILTER_ACCEPT: verdict = NF_ACCEPT; break;
			case FILTER_DROP:   verdict = NF_DROP; break;
			default:
				LOG_DEBUG(nfqueue_section, "unknown verdict");
				verdict = NF_DROP;
				break;
			}

			if (result == FILTER_ACCEPT && vbuffer_ismodified(&pkt->core_packet.payload)) {
				ret = nfq_set_verdict(pkt->state->queue, pkt->id, verdict, len, (uint8 *)data);
			}
			else {
				ret = nfq_set_verdict(pkt->state->queue, pkt->id, verdict, 0, NULL);
			}
		}

		if (pcap && result == FILTER_ACCEPT) {
			dump_pcap(&pcap->out, pkt, data, len);
		}

		if (ret == -1) {
			LOG_ERROR(nfqueue_section, "packet verdict failed");
		}

		vbuffer_clear(&pkt->core_packet.payload);
	}
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	return pkt->id;
}

static const char *packet_get_dissector(struct packet *pkt)
{
	return "ipv4";
}

static void packet_do_release(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;

	if (vbuffer_isvalid(&orig_pkt->payload)) {
		packet_verdict(orig_pkt, FILTER_DROP);
	}

	vbuffer_release(&pkt->core_packet.payload);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;

	if (vbuffer_isvalid(&orig_pkt->payload)) {
		if (pkt->id == -1)
			return STATUS_FORGED;
		else
			return STATUS_NORMAL;
	}
	else {
		return STATUS_SENT;
	}
}

static struct packet *new_packet(struct packet_module_state *state, size_t size)
{
	struct nfqueue_packet *packet = malloc(sizeof(struct nfqueue_packet));
	if (!packet) {
		error("Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct nfqueue_packet));

	if (!vbuffer_create_new(&packet->core_packet.payload, size, true)) {
		assert(check_error());
		free(packet);
		return NULL;
	}

	packet->id = -1;
	packet->state = state;
	time_gettimestamp(&packet->timestamp);

	return (struct packet *)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	bool ret;
	const uint8 *data;
	size_t len;

	data = vbuffer_flatten(&pkt->core_packet.payload, &len);
	if (!data) {
		assert(check_error());
		vbuffer_clear(&pkt->core_packet.payload);
		return false;
	}

	ret = socket_send_packet(pkt->state->send_fd, data, len);

	vbuffer_clear(&pkt->core_packet.payload);
	return ret;
}

static size_t get_mtu(struct packet *pkt)
{
	// TODO
	return 1500;
}

static const struct time *get_timestamp(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	return &pkt->timestamp;
}

static bool is_realtime()
{
	return true;
}


struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        "nfqueue",
		description: "Netfilter queue packet module",
		api_version: HAKA_API_VERSION,
		init:		 init,
		cleanup:	 cleanup
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
