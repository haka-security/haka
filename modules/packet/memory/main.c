/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pcap.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/parameters.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/engine.h>
#include <haka/container/list.h>
#include <haka/pcap.h>

struct pcap_packet {
	struct packet               core_packet;
	struct list                 list;
	struct time                 timestamp;
	struct packet_module_state *state;
	uint64                      id;
	bool                        captured;
	int                         protocol;
};

struct packet_module_state {
	struct pcap_capture         pd;
	pcap_dumper_t              *pf;
	uint64                      packet_id;
	struct pcap_packet         *received_head;
	struct pcap_packet         *current;
	struct pcap_packet         *received_tail;
};

/* Init parameters */
static char  *input_file;
static char  *output_file;
static bool   passthrough = true;
static int    repeat = 0;

static void cleanup()
{
	free(input_file);
	free(output_file);
}

static int init(struct parameters *args)
{
	const char *input, *output;

	if ((input = parameters_get_string(args, "file", NULL))) {
		input_file = strdup(input);
	}
	else {
		messagef(HAKA_LOG_ERROR, L"memory", L"missing input parameter");
		cleanup();
		return 1;
	}

	if ((output = parameters_get_string(args, "output", NULL))) {
		output_file = strdup(output);
	}

	passthrough = parameters_get_boolean(args, "pass-through", true);
	repeat = parameters_get_integer(args, "repeat", 0);

	return 0;
}

static bool multi_threaded()
{
	return true;
}

static bool pass_through()
{
	return passthrough;
}

static void cleanup_state(struct packet_module_state *state)
{
	if (state->pf) {
		pcap_dump_close(state->pf);
		state->pf = NULL;
	}

	if (state->pd.pd) {
		pcap_close(state->pd.pd);
	}

	free(state);
}

static bool load_packet(struct packet_module_state *state)
{
	int ret;
	struct pcap_pkthdr *header;
	const u_char *p;
	struct vbuffer data;
	struct vbuffer_sub sub;
	size_t data_offset;

	ret = pcap_next_ex(state->pd.pd, &header, &p);
	if (ret == -1) {
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", pcap_geterr(state->pd.pd));
		return 1;
	}
	else if (ret == -2) {
		/* end of pcap file */
		return 1;
	}
	else if (ret == 0) {
		/* Timeout expired. */
		return 0;
	}
	else if (header->caplen == 0 ||
			header->len < header->caplen) {
		messagef(HAKA_LOG_ERROR, L"memory", L"skipping malformed packet %d", ++state->packet_id);
		return 0;
	}
	else {
		struct pcap_packet *packet = malloc(sizeof(struct pcap_packet));
		if (!packet) {
			return ENOMEM;
		}

		memset(packet, 0, sizeof(struct pcap_packet));

		if (!vbuffer_create_from(&data, (char *)p, header->caplen)) {
			free(packet);
			return ENOMEM;
		}

		vbuffer_setwritable(&data, !passthrough);

		/* fill packet data structure */
		packet->state = state;
		packet->captured = true;
		packet->id = ++state->packet_id;
		packet->timestamp.secs = header->ts.tv_sec;
		packet->timestamp.nsecs = header->ts.tv_usec*1000;

		if (header->caplen < header->len)
			messagef(HAKA_LOG_WARNING, L"memory", L"packet truncated");

		if (state->pd.file) {
			const size_t cur = ftell(state->pd.file);
			const float percent = ((cur * 10000) / state->pd.file_size) / 100.f;
			struct time time, difftime;
			time_gettimestamp(&time);

			if (time_isvalid(&state->pd.last_progress)) {
				time_diff(&difftime, &time, &state->pd.last_progress);

				if (difftime.secs >= PROGRESS_DELAY) /* 5 seconds */
				{
					state->pd.last_progress = time;
					if (percent > 0) {
						messagef(HAKA_LOG_INFO, L"memory", L"progress %.2f %%", percent);
					}
				}
			}
			else {
				state->pd.last_progress = time;
			}
		}


		packet->protocol = get_protocol(state->pd.link_type, &data, &data_offset);

		vbuffer_sub_create(&sub, &data, data_offset, ALL);
		ret = vbuffer_select(&sub, &packet->core_packet.payload, NULL);
		vbuffer_release(&data);
		if (!ret) {
			messagef(HAKA_LOG_ERROR, L"pcap", L"malformed packet %d", packet->id);
			free(packet);
			return ENOMEM;
		}


		/* Finally insert packet in list */
		list_init(packet);
		if (state->received_head) {
			list_insert_after(packet, state->received_tail, &state->received_head, &state->received_tail);
		} else {
			state->received_head = packet;
			state->current = packet;
		}
		state->received_tail = packet;

		return 0;
	}
}

static bool load_pcap(struct packet_module_state *state, const char *input)
{
	size_t cur;
	char errbuf[PCAP_ERRBUF_SIZE];
	bzero(errbuf, PCAP_ERRBUF_SIZE);

	assert(input);

	messagef(HAKA_LOG_INFO, L"memory", L"opening file '%s'", input);

	state->pd.pd = pcap_open_offline(input, errbuf);

	if (!state->pd.pd) {
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", errbuf);
		return false;
	}

	state->pd.file = pcap_file(state->pd.pd);
	assert(state->pd.file);

	cur = ftell(state->pd.file);
	fseek(state->pd.file, 0L, SEEK_END);
	state->pd.file_size = ftell(state->pd.file);
	fseek(state->pd.file, cur, SEEK_SET);

	/* Determine the datalink layer type. */
	if ((state->pd.link_type = pcap_datalink(state->pd.pd)) < 0)
	{
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", pcap_geterr(state->pd.pd));
		return false;
	}

	/* Check for supported datalink layer. */
	switch (state->pd.link_type)
	{
	case DLT_EN10MB:
	case DLT_NULL:
	case DLT_LINUX_SLL:
	case DLT_IPV4:
	case DLT_RAW:
		break;

	case DLT_SLIP:
	case DLT_PPP:
	default:
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", "unsupported data link");
		return false;
	}

	messagef(HAKA_LOG_INFO, L"memory", L"loading packet in memory", input);
	while(load_packet(state) == 0);

	return true;
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		error(L"memory error");
		return NULL;
	}

	bzero(state, sizeof(struct packet_module_state));

	if (!load_pcap(state, input_file)) {
		cleanup_state(state);
		return NULL;
	}

	if (output_file) {
		/* open pcap savefile */
		state->pf = pcap_dump_open(state->pd.pd, output_file);
		if (!state->pf) {
			cleanup_state(state);
			messagef(HAKA_LOG_ERROR, L"memory", L"unable to dump on %s", output_file);
			return NULL;
		}
	}

	state->packet_id = 0;

	return state;
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	if (!state->current) {
		if (repeat > 0) {
			messagef(HAKA_LOG_INFO, L"memory", L"repeating input");
			state->current = state->received_head;
			repeat--;
		} else {
			/* No more packet */
			return 1;
		}
	}

	*pkt = (struct packet *)state->current;
	state->current = list_next(state->current);
	return 0;

}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	/* Nothing to do */
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	switch (pkt->protocol) {
	case ETH_P_IP:
		return "ipv4";

	case -1:
		messagef(HAKA_LOG_ERROR, L"pcap", L"malformed packet %d", pkt->id);

	default:
		return NULL;
	}
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return pkt->id;
}

static void packet_do_release(struct packet *orig_pkt)
{
	/* Nothing to do */
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (pkt->captured)
		return STATUS_NORMAL;
	else
		return STATUS_FORGED;
}

static struct packet *new_packet(struct packet_module_state *state, size_t size)
{
	struct pcap_packet *packet = malloc(sizeof(struct pcap_packet));
	if (!packet) {
		error(L"Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct pcap_packet));

	list_init(packet);
	packet->state = state;
	packet->captured = false;
	time_gettimestamp(&packet->timestamp);
	packet->id = 0;

	if (!vbuffer_create_new(&packet->core_packet.payload, size, true)) {
		assert(check_error());
		free(packet);
		return NULL;
	}

	return (struct packet *)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	error(L"sending is not supported in pass-through");
	return false;
}

static size_t get_mtu(struct packet *pkt)
{
	return 1500;
}

static const struct time *get_timestamp(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return &pkt->timestamp;
}

static bool is_realtime()
{
	return false;
}

struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        L"Memory Module",
		description: L"Packet capture from memory module",
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
