
#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/parameters.h>
#include <haka/thread.h>
#include <haka/error.h>
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

struct pcap_packet;

struct packet_module_state {
	pcap_t                     *pd;
	pcap_dumper_t              *pf;
	int                         link_type;
	uint64                      packet_id;
	struct pcap_packet         *sent_head;
	struct pcap_packet         *sent_tail;
};

struct pcap_packet {
	struct packet               core_packet;
	struct list                 list;
	struct packet_module_state *state;
	struct pcap_pkthdr          header;
	u_char                     *data;
	uint64                      id;
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
static char *input_file;
static char *input_iface;
static char *output_file;


static void cleanup()
{
	free(input_iface);
	free(input_file);
	free(output_file);
}

static int init(struct parameters *args)
{
	const char *input, *output, *interface;

	interface = parameters_get_string(args, "interfaces", NULL);
	input = parameters_get_string(args, "file", NULL);

	if ((interface && input) || !(interface || input)) {
		messagef(HAKA_LOG_ERROR, L"pcap", L"specifiy either a device or a pcap filename");
		cleanup();
		return 1;
	}

	if (interface)
		input_iface = strdup(interface);

	if (input)
		input_file = strdup(input);

	if ((output = parameters_get_string(args, "output", NULL)))
		output_file = strdup(output);

	return 0;
}

static bool multi_threaded()
{
	return false;
}

static void cleanup_state(struct packet_module_state *state)
{
	if (state->pf) {
		pcap_dump_close(state->pf);
		state->pf = NULL;
	}

	if (state->pd) {
		pcap_close(state->pd);
		state->pd = NULL;
	}

	free(state);
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;
	char errbuf[PCAP_ERRBUF_SIZE];

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		return NULL;
	}

	bzero(state, sizeof(struct packet_module_state));
	bzero(errbuf, PCAP_ERRBUF_SIZE);

	/* get a pcap descriptor from device */
	if (input_iface) {
		state->pd = pcap_open_live(input_iface, SNAPLEN, 1, 0, errbuf);
		if (state->pd && (strlen(errbuf) > 0)) {
			messagef(HAKA_LOG_WARNING, L"pcap", L"%s", errbuf);
		}
	}
	/* get a pcap descriptor from a pcap file */
	else if (state) {
		state->pd = pcap_open_offline(input_file, errbuf);
	}
	else {
		/* This case should never be reached */
		assert(0);
	}

	if (!state->pd) {
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errbuf);
		cleanup_state(state);
		return NULL;
	}

	if (output_file) {
		/* open pcap savefile */
		state->pf = pcap_dump_open(state->pd, output_file);
		if (!state->pf) {
			cleanup_state(state);
			messagef(HAKA_LOG_ERROR, L"pcap", L"unable to dump on %s", output_file);
			return NULL;
		}
	}

	/* Determine the datalink layer type. */
	if ((state->link_type = pcap_datalink(state->pd)) < 0)
	{
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(state->pd));
		cleanup_state(state);
		return NULL;
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
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", "unsupported data link");
		cleanup_state(state);
		return NULL;
	}

	state->packet_id = 0;

	return state;
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	struct pcap_pkthdr *header;
	const u_char *p;
	int ret;

	/* first check if a packet is waiting in the sent queue */
	if (state->sent_head) {
		struct pcap_packet *packet = state->sent_head;

		list_remove(packet, &state->sent_head, &state->sent_tail);
		packet->captured = true;

		*pkt = (struct packet *)packet;
		return 0;
	}
	else {
		/* read packet */
		ret = pcap_next_ex(state->pd, &header, &p);

		if(ret == -1) {
			/* error while reading packet */
			messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(state->pd));
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
		else {
			struct pcap_packet *packet = malloc(sizeof(struct pcap_packet));
			if (!packet) {
				return ENOMEM;
			}

			memset(packet, 0, sizeof(struct pcap_packet));

			list_init(packet);

			packet->data = malloc(header->caplen);
			if (!packet->data) {
				free(packet);
				return ENOMEM;
			}

			/* fill packet data structure */
			memcpy(packet->data, p, header->caplen);
			packet->header = *header;
			packet->state = state;
			packet->captured = true;
			packet->id = state->packet_id++;

			if (packet->header.caplen < packet->header.len)
				messagef(HAKA_LOG_WARNING, L"pcap", L"packet truncated");

			*pkt = (struct packet *)packet;
			return 0;
		}
	}
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	/* dump capture in pcap file */
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (pkt->data) {
		if (pkt->state->pf && result == FILTER_ACCEPT)
			pcap_dump((u_char *)pkt->state->pf, &(pkt->header), pkt->data);

		free(pkt->data);
		pkt->data = NULL;
	}
}

static int get_protocol(struct pcap_packet *pkt, size_t *data_offset)
{
	switch (pkt->state->link_type)
	{
	case DLT_LINUX_SLL:
		{
			struct linux_sll_header *eh = (struct linux_sll_header *)pkt->data;
			*data_offset = sizeof(struct linux_sll_header);
			if (eh) return ntohs(eh->protocol);
			else return 0;
		}
		break;

	case DLT_EN10MB:
		{
			struct ethhdr *eh = (struct ethhdr *)pkt->data;
			*data_offset = sizeof(struct ethhdr);
			if (eh) return ntohs(eh->h_proto);
			else return 0;
		}
		break;

	case DLT_IPV4:
	case DLT_RAW:
		*data_offset = 0;
		return ETH_P_IP;

		{
			*data_offset = 16;
			struct ethhdr *eh = (struct ethhdr *)pkt->data;
			*data_offset = sizeof(struct ethhdr);
			if (eh) return ntohs(eh->h_proto);
			else return 0;
		}
		break;

	case DLT_NULL:
		*data_offset = 4;
		if (*(uint32 *)pkt->data == PF_INET) {
			return ETH_P_IP;
		}
		else {
			return -1;
		}
		break;

	default:
		*data_offset = 0;
		assert(0);
		return -1;
	}
}

static size_t packet_get_length(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	size_t data_offset;
	get_protocol(pkt, &data_offset);
	return pkt->header.caplen - data_offset;
}

static const uint8 *packet_get_data(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	size_t data_offset;
	get_protocol(pkt, &data_offset);
	return (pkt->data + data_offset);
}

static uint8 *packet_modifiable(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	size_t data_offset;
	get_protocol(pkt, &data_offset);
	return (pkt->data + data_offset);
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

static int packet_do_resize(struct packet *orig_pkt, size_t size)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	u_char *new_data;
	size_t new_size, copy_size;
	size_t data_offset;

	get_protocol(pkt, &data_offset);
	new_size = size + data_offset;
	copy_size = MIN(new_size, pkt->header.caplen);

	new_data = malloc(new_size);
	if (!new_data) {
		error(L"memory error");
		return ENOMEM;
	}

	memcpy(new_data, pkt->data, copy_size);
	memset(new_data + copy_size, 0, new_size - copy_size);

	free(pkt->data);
	pkt->data = new_data;
	pkt->header.len = new_size;
	pkt->header.caplen = new_size;
	return 0;
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return pkt->id;
}

static void packet_do_release(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (pkt->data) {
		packet_verdict(orig_pkt, FILTER_DROP);
	}

	free(pkt->data);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (pkt->data && !list_next(pkt) && !list_prev(pkt)) {
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
	size_t data_offset;
	struct pcap_packet *packet = malloc(sizeof(struct pcap_packet));
	if (!packet) {
		error(L"Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct pcap_packet));

	list_init(packet);
	packet->state = state;
	packet->captured = false;

	{
		const time_us timestamp = time_gettimestamp();
		packet->header.ts.tv_sec = timestamp / 1000000;
		packet->header.ts.tv_usec = timestamp % 1000000;
	}

	get_protocol(packet, &data_offset);
	size += data_offset;

	packet->data = malloc(size);
	if (!packet->data) {
		free(packet);
		error(L"Memory error");
		return NULL;
	}

	memset(packet->data, 0, size);

	packet->header.len = size;
	packet->header.caplen = size;
	packet->id = state->packet_id++;

	switch (state->link_type)
	{
	case DLT_IPV4:
	case DLT_RAW:
		break;

	case DLT_NULL:
		*(uint32 *)packet->data = PF_INET;
		break;

	case DLT_LINUX_SLL:
		{
			struct linux_sll_header *eh = (struct linux_sll_header *)packet->data;
			eh->protocol = htons(ETH_P_IP);
		}
		break;

	case DLT_EN10MB:
		{
			struct ethhdr *eh = (struct ethhdr *)packet->data;
			eh->h_proto = htons(ETH_P_IP);
		}
		break;

	default:
		assert(0);
		break;
	}

	return (struct packet *)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	assert(pkt->data);
	assert(!list_next(pkt) && !list_prev(pkt));

	list_insert_after(pkt, pkt->state->sent_tail, &pkt->state->sent_head, &pkt->state->sent_tail);

	return true;
}

static size_t get_mtu(struct packet *pkt)
{
	return 1500;
}

static time_us get_timestamp(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return (((time_us)pkt->header.ts.tv_sec)*1000000) + pkt->header.ts.tv_usec;
}


struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        L"Pcap Module",
		description: L"Pcap packet module",
		author:      L"Arkoon Network Security",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
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
	packet_getstate: packet_getstate,
	new_packet:      new_packet,
	send_packet:     send_packet,
	get_mtu:         get_mtu,
	get_timestamp:   get_timestamp
};
