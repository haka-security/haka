
#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/error.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#include "iptables.h"
#include <pcap.h>
#include "config.h"

mutex_t ct_mutex = PTHREAD_MUTEX_INITIALIZER;

#define PACKET_BUFFER_SIZE	0xffff
/* Should be enough to receive the packet and the extra headers */
#define PACKET_RECV_SIZE	70000

#define MAX_INTERFACE 8


struct pcap_dump {
	pcap_t        *pd;
	pcap_dumper_t *pf;
};

struct pcap_sinks {
	struct pcap_dump   in;
	struct pcap_dump   out;
	struct pcap_dump   drop;
};

struct packet_module_state {
	struct nfq_handle           *handle;
	struct nfq_q_handle         *queue;
	int                          fd;

	struct nfqueue_packet       *current_packet; /* Packet allocated by nfq callback */
	int                          error;
	char                         receive_buffer[PACKET_RECV_SIZE];
};

static struct pcap_sinks           *pcap = NULL;

struct nfqueue_packet {
	struct packet               core_packet;
	struct packet_module_state *state;
	int                         id; /* nfq identifier */
	size_t                      length;
	int                         modified:1;
	uint8                       *data;
};

bool use_multithreading = true;

/* Iptables rules to add (iptables-restore format) */
static const char iptables_config_template_begin[] =
"*raw\n"
":PREROUTING ACCEPT [0:0]\n"
":OUTPUT ACCEPT [0:0]\n"
;

static const char iptables_config_template_mt_iface[] =
"-A PREROUTING -i %s -m mark --mark 0xffff -j ACCEPT\n"
"-A PREROUTING -i %s -j NFQUEUE --queue-balance 0:%i\n"
"-A OUTPUT -o %s -j MARK --set-mark 0xffff\n"
"-A OUTPUT -o %s -j NFQUEUE --queue-balance 0:%i\n"
;

static const char iptables_config_template_iface[] =
"-A PREROUTING -i %s -m mark --mark 0xffff -j ACCEPT\n"
"-A PREROUTING -i %s -j NFQUEUE\n"
"-A OUTPUT -o %s -j MARK --set-mark 0xffff\n"
"-A OUTPUT -o %s -j NFQUEUE\n"
;

static const char iptables_config_template_mt_all[] =
"-A PREROUTING -m mark --mark 0xffff -j ACCEPT\n"
"-A PREROUTING -j NFQUEUE --queue-balance 0:%i\n"
"-A OUTPUT -j MARK --set-mark 0xffff\n"
"-A OUTPUT -j NFQUEUE --queue-balance 0:%i\n"
;

static const char iptables_config_template_all[] =
"-A PREROUTING -m mark --mark 0xffff -j ACCEPT\n"
"-A PREROUTING -j NFQUEUE\n"
"-A OUTPUT -j MARK --set-mark 0xffff\n"
"-A OUTPUT -j NFQUEUE\n"
;

static const char iptables_config_template_end[] =
"COMMIT\n"
;

static int iptables_config_build(char *output, size_t outsize, char **ifaces, int threads)
{
	int size, total_size = 0;

	size = snprintf(output, outsize, iptables_config_template_begin);
	if (!size) return -1;
	if (output) { output += size; outsize-=size; }
	total_size += size;

	if (ifaces) {
		char **iface = ifaces;
		while (*iface) {
			if (threads > 1) {
				size = snprintf(output, outsize, iptables_config_template_mt_iface, *iface, *iface,
						threads-1, *iface, *iface, threads-1);
			}
			else {
				size = snprintf(output, outsize, iptables_config_template_iface, *iface, *iface,
						*iface, *iface);
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

	size = snprintf(output, outsize, iptables_config_template_end);
	if (!size) return -1;
	if (output) { output += size; outsize-=size; }
	total_size += size;

	return total_size;
}

static char *iptables_config(char **ifaces, int threads)
{
	char *new_iptables_config;

	const int size = iptables_config_build(NULL, 0, ifaces, threads);
	if (size <= 0) {
		return NULL;
	}

	new_iptables_config = malloc(sizeof(char) * (size+1));
	if (!new_iptables_config) {
		return NULL;
	}

	iptables_config_build(new_iptables_config, size+1, ifaces, threads);
	return new_iptables_config;
}

/* Iptables raw table current configuration */
static char *iptables_saved;


static int packet_callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		struct nfq_data *nfad, void *data)
{
	struct packet_module_state *state = data;
	struct nfqnl_msg_packet_hdr* packet_hdr;
	char *packet_data;
	int packet_len;

	state->error = 0;

	packet_hdr = nfq_get_msg_packet_hdr(nfad);
	if (!packet_hdr) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"unable to get packet header");
		return 0;
	}

	packet_len = nfq_get_payload(nfad, &packet_data);
	if (packet_len > PACKET_BUFFER_SIZE) {
		message(HAKA_LOG_WARNING, MODULE_NAME, L"received packet is too large");
		return 0;
	}
	else if (packet_len < 0) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"unable to get packet payload");
		return 0;
	}

	state->current_packet = malloc(sizeof(struct nfqueue_packet));
	if (!state->current_packet) {
		state->error = ENOMEM;
		return 0;
	}

	memset(state->current_packet,0,sizeof(struct nfqueue_packet));

	state->current_packet->data = malloc(packet_len);
	if (!state->current_packet->data) {
		free(state->current_packet);
		state->error = ENOMEM;
		return 0;
	}

	state->current_packet->length = packet_len;
	state->current_packet->modified = 0;
	state->current_packet->id = ntohl(packet_hdr->packet_id);

	/* The copy is needed as the packet buffer will be overridden when the next
	 * packet will arrive.
	 */
	memcpy(state->current_packet->data, packet_data, packet_len);

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

	free(state);
}

static struct packet_module_state *init_state(int thread_id)
{
	static const u_int16_t proto_family[] = { AF_INET, AF_INET6 };
	int i;
	struct packet_module_state *state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		return NULL;
	}

	/* Setup nfqueue connection */
	state->handle = nfq_open();
	if (!state->handle) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"unable to open nfqueue handle");
		cleanup_state(state);
		return NULL;
	}

	for (i=0; i<sizeof(proto_family)/sizeof(proto_family[0]); ++i) {
		if (nfq_unbind_pf(state->handle, proto_family[i]) < 0) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot unbind queue");
			cleanup_state(state);
			return NULL;
		}

		if (nfq_bind_pf(state->handle, proto_family[i]) < 0) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot bind queue");
			cleanup_state(state);
			return NULL;
		}
	}

	state->queue = nfq_create_queue(state->handle, thread_id,
			&packet_callback, state);
	if (!state->queue) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot create queue");
		cleanup_state(state);
		return NULL;
	}

	if (nfq_set_mode(state->queue, NFQNL_COPY_PACKET,
			PACKET_BUFFER_SIZE) < 0) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot set mode to copy packet");
		cleanup_state(state);
		return NULL;
	}

	state->fd = nfq_fd(state->handle);

	return state;
}

static int open_pcap(struct pcap_dump *pcap, const char *file)
{
	if (file) {
		pcap->pd = pcap_open_dead(DLT_IPV4, PACKET_RECV_SIZE);
		if (!pcap->pd) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot setup pcap sink");
			return 1;
		}

		pcap->pf = pcap_dump_open(pcap->pd, file);
		if (!pcap->pf) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot setup pcap sink");
			return 1;
		}
	}

	return 0;
}

static void close_pcap(struct pcap_dump *pcap)
{
	if (pcap->pf) pcap_dump_close(pcap->pf);
	if (pcap->pd) pcap_close(pcap->pd);
}

static void cleanup()
{
	if (iptables_saved) {
		if (apply_iptables(iptables_saved) != 0) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot restore iptables rules");
		}
		free(iptables_saved);
	}

	if (pcap) {
		close_pcap(&pcap->in);
		close_pcap(&pcap->out);
		close_pcap(&pcap->drop);
		free(pcap);
		pcap = NULL;
	}
}

static int init(int argc, char *argv[])
{
	char *new_iptables_config = NULL;
	char **ifaces = NULL;
	int thread_count = thread_get_packet_capture_cpu_count();
	const char *file_in = NULL;
	const char *file_out = NULL;
	const char *file_drop = NULL;
	int dump = 0;

	/* Setup iptables rules */
	if (save_iptables("raw", &iptables_saved)) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot save iptables rules");
		cleanup();
		return 1;
	}

	if (argc > 0) {
		if (strcmp(argv[0], "--single") == 0) {
			use_multithreading = false;
			thread_count = 1;

			argc -= 1;
			argv += 1;
		}
	}

	if (argc > 0) {
		if (strcmp(argv[0], "-p") == 0) {
			if (argc < 4) {
				message(HAKA_LOG_ERROR, MODULE_NAME, L"missing parameters for pcap dump");
				cleanup();
				return 1;
			}

			dump = 1;
			file_in = argv[1];
			file_out = argv[2];
			file_drop = argv[3];

			argc -= 4;
			argv += 4;
		}
	}

	if (argc > 0) {
		if (strcmp(argv[0], "any") != 0) {
			int index = 0;

			ifaces = malloc((sizeof(char *) * (argc+1)));
			if (!ifaces) {
				message(HAKA_LOG_ERROR, MODULE_NAME, L"memory error");
				cleanup();
				return 1;
			}

			while (index < argc) {
				ifaces[index] = argv[index];
				index++;
			}
			ifaces[index] = NULL;
		}
	}

	new_iptables_config = iptables_config(ifaces, thread_count);
	if (!new_iptables_config) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot generate iptables rules");
		if (ifaces)
			free(ifaces);
		cleanup();
		return 1;
	}
	free(ifaces);
	ifaces = NULL;

	if (apply_iptables(new_iptables_config)) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot setup iptables rules");
		free(new_iptables_config);
		cleanup();
		return 1;
	}

	free(new_iptables_config);

	/* Setup pcap dump */
	if (dump) {
		pcap = malloc(sizeof(struct pcap_sinks));
		if (!pcap) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"memory error");
			cleanup();
			return 1;
		}

		memset(pcap, 0, sizeof(struct pcap_sinks));

		open_pcap(&pcap->in, file_in);
		open_pcap(&pcap->out, file_out);
		open_pcap(&pcap->drop, file_drop);
	}
	return 0;
}

static bool multi_threaded()
{
	return use_multithreading;
}

static void dump_pcap(struct pcap_dump *pcap, struct nfqueue_packet *pkt)
{
	if (pcap->pf) {
		struct pcap_pkthdr hdr;

		mutex_lock(&ct_mutex);

		hdr.caplen = hdr.len = pkt->length;
		hdr.ts.tv_sec = 0;
		hdr.ts.tv_usec = 0;
		pcap_dump((u_char *)pcap->pf, &hdr, pkt->data);

		mutex_unlock(&ct_mutex);
	}
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	const int rv = recv(state->fd, state->receive_buffer,
			sizeof(state->receive_buffer), 0);
	if (rv < 0) {
		messagef(HAKA_LOG_ERROR, MODULE_NAME, L"packet reception failed, %s", errno_error(errno));
		return 0;
	}

	if (nfq_handle_packet(state->handle, state->receive_buffer, rv) == 0) {
		if (state->current_packet) {
			state->current_packet->state = state;
			*pkt = (struct packet*)state->current_packet;

			if (pcap) {
				dump_pcap(&pcap->in, state->current_packet);
			}

			state->current_packet = NULL;
			return 0;
		}
		else {
			return state->error;
		}
	}
	else {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"packet processing failed");
		return 0;
	}
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	int ret;
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;

	if (pkt->data) {
		/* Convert verdict to netfilter */
		int verdict;
		switch (result) {
		case FILTER_ACCEPT: verdict = NF_ACCEPT; break;
		case FILTER_DROP:   verdict = NF_DROP; break;
		default:
			message(HAKA_LOG_DEBUG, MODULE_NAME, L"unknown verdict");
			verdict = NF_DROP;
			break;
		}

		if (pkt->modified)
			ret = nfq_set_verdict(pkt->state->queue, pkt->id, verdict, pkt->length, pkt->data);
		else
			ret = nfq_set_verdict(pkt->state->queue, pkt->id, verdict, 0, NULL);

		if (pcap) {
			switch (result) {
			case FILTER_ACCEPT:
				dump_pcap(&pcap->out, pkt);
				break;

			case FILTER_DROP:
			default:
				dump_pcap(&pcap->drop, pkt);
				break;
			}
		}

		if (ret == -1) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"packet verdict failed");
		}

		free(pkt->data);
		pkt->data = NULL;
	}
}

static size_t packet_get_length(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	return pkt->length;
}

static const uint8 *packet_get_data(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	return pkt->data;
}

static uint8 *packet_modifiable(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	pkt->modified = 1;
	return pkt->data;
}

static int packet_do_resize(struct packet *orig_pkt, size_t size)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;
	uint8 *new_data;
	const size_t copy_size = MIN(size, pkt->length);

	new_data = malloc(size);
	if (!new_data) {
		error(L"memory error");
		return ENOMEM;
	}

	memcpy(new_data, pkt->data, copy_size);

	free(pkt->data);
	pkt->data = new_data;
	pkt->length = size;
	return 0;
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

	if (pkt->data) {
		packet_verdict(orig_pkt, FILTER_DROP);
	}

	free(pkt->data);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct nfqueue_packet *pkt = (struct nfqueue_packet*)orig_pkt;

	if (pkt->data) {
		return STATUS_NORMAL;
	}
	else {
		return STATUS_SENT;
	}
}


struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        MODULE_NAME,
		description: L"Netfilter queue packet module",
		author:		 L"Arkoon Network Security",
		api_version: HAKA_API_VERSION,
		init:		 init,
		cleanup:	 cleanup
	},
	multi_threaded:  multi_threaded,
	init_state:      init_state,
	cleanup_state:   cleanup_state,
	receive:         packet_do_receive,
	verdict:         packet_verdict,
	get_length:      packet_get_length,
	make_modifiable: packet_modifiable,
	resize:          packet_do_resize,
	get_id:          packet_get_id,
	get_data:        packet_get_data,
	get_dissector:   packet_get_dissector,
	release_packet:  packet_do_release,
	packet_getstate: packet_getstate
};
