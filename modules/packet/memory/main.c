/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/parameters.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/engine.h>
#include <haka/container/list.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pcap.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>


/* snapshot length - a value of 65535 is sufficient to get all
 * of the packet's data on most networks (from pcap man page)
 */
#define SNAPLEN 65535

#define PROGRESS_DELAY      5 /* 5 seconds */

struct pcap_packet;

struct packet_module_state {
	pcap_t                     *pd;
	int                         link_type;
	FILE                       *file;
	size_t                      file_size;
	struct time                 last_progress;
	pcap_dumper_t              *pf;
	uint64                      packet_id;
	struct pcap_packet         *received_head;
	struct pcap_packet         *current;
	struct pcap_packet         *received_tail;
	struct pcap_packet         *sent_head;
	struct pcap_packet         *sent_tail;
};

struct pcap_packet {
	struct packet               core_packet;
	struct list                 list;
	struct time                 timestamp;
	struct packet_module_state *state;
	struct pcap_pkthdr          header;
	struct vbuffer              data;
	struct vbuffer_iterator     select;
	uint64                      id;
	int                         link_type;
	bool                        captured;
};

/*
 * Packet headers
 */

struct linux_sll_header {
	uint16     type;
	uint16     arphdr_type;
	uint16     link_layer_length;
	uint64     link_layer_address;
	uint16     protocol;
} PACKED;


/* Init parameters */
static char  *input_file;
static char  *output_file;
static bool   passthrough = true;


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

	pcap_close(state->pd);

	free(state);
}

static bool load_packet(struct packet_module_state *state)
{
	int ret;
	struct pcap_pkthdr *header;
	const u_char *p;

	ret = pcap_next_ex(state->pd, &header, &p);
	if (ret == -1) {
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", pcap_geterr(state->pd));
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

		list_init(packet);
		if (state->received_head) {
			list_insert_after(packet, state->received_tail, &state->received_head, &state->received_tail);
		} else {
			state->received_head = packet;
			state->current = packet;
		}
		state->received_tail = packet;

		if (!vbuffer_create_from(&packet->data, (char *)p, header->caplen)) {
			free(packet);
			return ENOMEM;
		}

		vbuffer_setwritable(&packet->data, !passthrough);

		/* fill packet data structure */
		packet->header = *header;
		packet->state = state;
		packet->captured = true;
		packet->link_type = state->link_type;
		packet->id = ++state->packet_id;
		packet->timestamp.secs = header->ts.tv_sec;
		packet->timestamp.nsecs = header->ts.tv_usec*1000;

		if (packet->header.caplen < packet->header.len)
			messagef(HAKA_LOG_WARNING, L"memory", L"packet truncated");

		if (state->file) {
			const size_t cur = ftell(state->file);
			const float percent = ((cur * 10000) / state->file_size) / 100.f;
			struct time time, difftime;
			time_gettimestamp(&time);

			if (time_isvalid(&state->last_progress)) {
				time_diff(&difftime, &time, &state->last_progress);

				if (difftime.secs >= PROGRESS_DELAY) /* 5 seconds */
				{
					state->last_progress = time;
					if (percent > 0) {
						messagef(HAKA_LOG_INFO, L"memory", L"progress %.2f %%", percent);
					}
				}
			}
			else {
				state->last_progress = time;
			}
		}

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

	state->pd = pcap_open_offline(input, errbuf);

	if (!state->pd) {
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", errbuf);
		return false;
	}

	state->file = pcap_file(state->pd);
	assert(state->file);

	cur = ftell(state->file);
	fseek(state->file, 0L, SEEK_END);
	state->file_size = ftell(state->file);
	fseek(state->file, cur, SEEK_SET);

	/* Determine the datalink layer type. */
	if ((state->link_type = pcap_datalink(state->pd)) < 0)
	{
		messagef(HAKA_LOG_ERROR, L"memory", L"%s", pcap_geterr(state->pd));
		return false;
	}

	/* Check for supported datalink layer. */
	switch (state->link_type)
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
		state->pf = pcap_dump_open(state->pd, output_file);
		if (!state->pf) {
			cleanup_state(state);
			messagef(HAKA_LOG_ERROR, L"memory", L"unable to dump on %s", output_file);
			return NULL;
		}
	}

	state->packet_id = 0;

	return state;
}

static int get_link_type_offset(struct pcap_packet *pkt)
{
	size_t size;

	switch (pkt->link_type)
	{
	case DLT_LINUX_SLL: size = sizeof(struct linux_sll_header); break;
	case DLT_EN10MB:    size = sizeof(struct ethhdr); break;
	case DLT_IPV4:
	case DLT_RAW:       size = 0; break;
	case DLT_NULL:      size = 4; break;

	default:            assert(!"unsupported link type"); return -1;
	}

	return size;
}

static int get_protocol(struct pcap_packet *pkt, size_t *data_offset)
{
	size_t len, size;
	struct vbuffer_sub sub;
	const uint8* data = NULL;

	size = get_link_type_offset(pkt);
	*data_offset = size;

	if (size > 0) {
		vbuffer_sub_create(&sub, &pkt->data, 0, size);
		data = vbuffer_sub_flatten(&sub, &len);

		if (len < *data_offset) {
			messagef(HAKA_LOG_ERROR, L"memory", L"malformed packet %d", pkt->id);
			return -1;
		}

		assert(data);
	}

	switch (pkt->link_type)
	{
	case DLT_LINUX_SLL:
		{
			struct linux_sll_header *eh = (struct linux_sll_header *)data;
			if (eh) return ntohs(eh->protocol);
			else return 0;
		}
		break;

	case DLT_EN10MB:
		{
			struct ethhdr *eh = (struct ethhdr *)data;
			if (eh) return ntohs(eh->h_proto);
			else return 0;
		}
		break;

	case DLT_IPV4:
	case DLT_RAW:
		return ETH_P_IP;

	case DLT_NULL:
		*data_offset = 4;
		if (*(uint32 *)data == PF_INET) {
			return ETH_P_IP;
		}
		else {
			return -1;
		}
		break;

	default:
		assert(!"unsupported link type");
		return -1;
	}
}

static bool packet_build_payload(struct pcap_packet *packet)
{
	struct vbuffer_sub sub;
	size_t data_offset;
	if (get_protocol(packet, &data_offset) < 0) {
		return false;
	}

	vbuffer_sub_create(&sub, &packet->data, data_offset, ALL);

	return vbuffer_select(&sub, &packet->core_packet.payload, &packet->select);
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	/* first check if a packet is waiting in the sent queue */
	if (state->sent_head) {
		struct pcap_packet *packet = state->sent_head;

		list_remove(packet, &state->sent_head, &state->sent_tail);
		packet->captured = true;

		*pkt = (struct packet *)packet;
		return 0;
	}
	else {
		if (!state->current) {
			/* No more packet */
			return 1;
		}

		if (!packet_build_payload(state->current)) {
			return ENOMEM;
		}

		*pkt = (struct packet *)state->current;
		state->current = list_next(state->current);
		return 0;

	}
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	/* dump capture in pcap file */
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data)) {
		vbuffer_restore(&pkt->select, &pkt->core_packet.payload, false);

		if (pkt->state->pf && result == FILTER_ACCEPT) {
			const uint8 *data;
			size_t len;

			data = vbuffer_flatten(&pkt->data, &len);
			if (!data) {
				assert(check_error());
				vbuffer_clear(&pkt->data);
				return;
			}

			if (pkt->header.caplen != len) {
				pkt->header.len = len;
				pkt->header.caplen = len;
			}

			pcap_dump((u_char *)pkt->state->pf, &(pkt->header), data);
		}

		vbuffer_clear(&pkt->data);
	}
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	size_t data_offset;
	switch (get_protocol(pkt, &data_offset)) {
	case ETH_P_IP:
		return "ipv4";

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
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data)) {
		packet_verdict(orig_pkt, FILTER_DROP);
	}

	vbuffer_release(&pkt->core_packet.payload);
	vbuffer_release(&pkt->data);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data) && !list_next(pkt) && !list_prev(pkt)) {
		if (pkt->captured)
			return STATUS_NORMAL;
		else
			return STATUS_FORGED;
	}
	else {
		return STATUS_SENT;
	}
}

static struct packet *new_packet(struct packet_module_state *state, size_t size)
{
	uint8 *data;
	size_t data_offset, len;
	struct vbuffer_sub sub;
	struct pcap_packet *packet = malloc(sizeof(struct pcap_packet));
	if (!packet) {
		error(L"Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct pcap_packet));

	list_init(packet);
	packet->state = state;
	packet->captured = false;
	packet->link_type = state->link_type;
	time_gettimestamp(&packet->timestamp);

	data_offset = get_link_type_offset(packet);
	size += data_offset;

	if (!vbuffer_create_new(&packet->data, size, true)) {
		assert(check_error());
		free(packet);
		return NULL;
	}

	vbuffer_sub_create(&sub, &packet->data, 0, data_offset);

	data = vbuffer_mmap(&sub, &len, true, NULL, NULL);
	assert(data || len == 0);
	assert(len == data_offset);

	packet->header.len = size;
	packet->header.caplen = size;
	packet->id = 0;

	switch (state->link_type)
	{
	case DLT_IPV4:
	case DLT_RAW:
		break;

	case DLT_NULL:
		*(uint32 *)data = PF_INET;
		break;

	case DLT_LINUX_SLL:
		{
			struct linux_sll_header *eh = (struct linux_sll_header *)data;
			eh->protocol = htons(ETH_P_IP);
		}
		break;

	case DLT_EN10MB:
		{
			struct ethhdr *eh = (struct ethhdr *)data;
			eh->h_proto = htons(ETH_P_IP);
		}
		break;

	default:
		assert(0);
		break;
	}

	if (!packet_build_payload(packet)) {
		vbuffer_release(&packet->data);
		free(packet);
		return NULL;
	}

	return (struct packet *)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (passthrough) {
		error(L"sending is not supported in pass-through");
		return false;
	}

	assert(vbuffer_isvalid(&pkt->data));
	assert(!list_next(pkt) && !list_prev(pkt));

	list_insert_after(pkt, pkt->state->sent_tail, &pkt->state->sent_head, &pkt->state->sent_tail);

	return true;
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
