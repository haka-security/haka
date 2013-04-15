
#include <haka/packet_module.h>
#include <haka/log.h>

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
#include "config.h"


#define PACKET_BUFFER_SIZE    0xffff
/* Should be enough to receive the packet and the extra headers */
#define PACKET_RECV_SIZE      70000


struct packet {
	int         id; /* nfq identifier */
	size_t      length;
	char        data[0];
};

/*
 * nfqueue state
 */
struct nfq_state {
	struct nfq_handle   *handle;
	struct nfq_q_handle *queue;
	int                  fd;

	struct packet       *current_packet; /* Packet allocated by nfq callback */
	int                  error;
	char                 receive_buffer[PACKET_RECV_SIZE];
};

/* Only one state for now */
static struct nfq_state   global_state;

/* Iptables rules to add (iptables-restore format) */
static const char iptables_config[] =
"*raw\n"
":PREROUTING DROP [0:0]\n"
":OUTPUT DROP [0:0]\n"
"-A PREROUTING -j NFQUEUE --queue-num 0\n"
"-A OUTPUT -j NFQUEUE --queue-num 0\n"
"COMMIT\n"
;

/* Iptables raw table current configuration */
static char *iptables_saved;


static int packet_callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		struct nfq_data *nfad, void *data)
{
	struct nfq_state *state = data;
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

	state->current_packet = malloc(sizeof(struct packet) + packet_len);
	if (!state->current_packet) {
		state->error = ENOMEM;
		return 0;
	}

	state->current_packet->length = packet_len;
	state->current_packet->id = ntohl(packet_hdr->packet_id);

	/* The copy is needed as the packet buffer will be overridden when the next
	 * packet will arrive.
	 */
	memcpy(state->current_packet->data, packet_data, packet_len);

	return 0;
}

static void cleanup()
{
	if (global_state.queue)
		nfq_destroy_queue(global_state.queue);
	if (global_state.handle)
		nfq_close(global_state.handle);

	if (iptables_saved) {
		if (apply_iptables(iptables_saved) != 0) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot restore iptables rules");
		}
		free(iptables_saved);
	}
}

static int init(int argc, char *argv[])
{
	static const u_int16_t proto_family[] = { AF_INET, AF_INET6 };
	int i;

	/* Setup nfqueue connection */
	global_state.handle = nfq_open();
	if (!global_state.handle) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"unable to open nfqueue handle");
		return 1;
	}

	for (i=0; i<sizeof(proto_family)/sizeof(proto_family[0]); ++i) {
		if (nfq_unbind_pf(global_state.handle, proto_family[i]) < 0) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot unbind queue");
			cleanup();
			return 1;
		}

		if (nfq_bind_pf(global_state.handle, proto_family[i]) < 0) {
			message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot bind queue");
			cleanup();
			return 1;
		}
	}

	global_state.queue = nfq_create_queue(global_state.handle, 0,
			&packet_callback, &global_state);
	if (!global_state.queue) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot create queue");
		cleanup();
		return 1;
	}

	if (nfq_set_mode(global_state.queue, NFQNL_COPY_PACKET,
			PACKET_BUFFER_SIZE) < 0) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot set mode to copy packet");
		cleanup();
		return 1;
	}

	global_state.fd = nfq_fd(global_state.handle);

	/* Setup iptables rules */
	if (save_iptables("raw", &iptables_saved)) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot save iptables rules");
		cleanup();
		return 1;
	}

	if (apply_iptables(iptables_config)) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot setup iptables rules");
		cleanup();
		return 1;
	}

	return 0;
}

static int packet_receive(struct packet **pkt)
{
	const int rv = recv(global_state.fd, global_state.receive_buffer,
			sizeof(global_state.receive_buffer), 0);
	if (rv < 0) {
		messagef(HAKA_LOG_ERROR, MODULE_NAME, L"packet reception failed, %s", strerror(errno));
		return 0;
	}

	if (nfq_handle_packet(global_state.handle, global_state.receive_buffer, rv) == 0) {
		if (global_state.current_packet) {
			*pkt = global_state.current_packet;
			global_state.current_packet = NULL;
			return 0;
		}
		else {
			return global_state.error;
		}
	}
	else {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"packet processing failed");
		return 0;
	}
}

static void packet_verdict(struct packet *pkt, filter_result result)
{
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

	if (nfq_set_verdict(global_state.queue, pkt->id, verdict, 0, NULL) == -1) {
		message(HAKA_LOG_ERROR, MODULE_NAME, L"packet verdict failed");
	}

	free(pkt);
}

static size_t packet_get_length(struct packet *pkt)
{
	return pkt->length;
}

static const char *packet_get_data(struct packet *pkt)
{
	return pkt->data;
}


/**
 * @defgroup NetfilterQueue Netfilter Queue
 * @brief Packet filtering using Netfilter Queue to interface with the Linux Kernel.
 * @author Arkoon Network Security
 * @ingroup ExternPacketModule
 *
 * # Description
 *
 * The module uses the library netfilter queue to intercept all packets.
 *
 * At initialization, the module will install some iptables rules for the _raw_ table.
 * The table will be cleared when the application is closed.
 *
 * To operate correctly, the process need to be launched with the proper access
 * rights.
 *
 * # Initialization arguments
 *
 * This module does not take any initialization parameters.
 *
 * # Usage
 *
 * Module usage.
 *
 * ### Lua
 * ~~~~~~~~{.lua}
 * app.install("packet", module.load("packet-nfqueue"))
 * ~~~~~~~~
 *
 * ### C/C++
 * ~~~~~~~~{.c}
 * module_load("packet-nfqueue", 0, NULL);
 * ~~~~~~~~
 */
struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        MODULE_NAME,
		description: L"Netfilter queue packet module",
		author:      L"Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	receive:         packet_receive,
	verdict:         packet_verdict,
	get_length:      packet_get_length,
	get_data:        packet_get_data
};
