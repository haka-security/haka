
#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/thread.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pcap.h>
#include <string.h>
#include <assert.h>


/* snapshot length - a value of 65535 is sufficient to get all
 * of the packet's data on most networks (from pcap man page)
 */
#define SNAPLEN 65535

struct packet_module_state {
	pcap_t                     *pd;
	pcap_dumper_t              *pf;
	size_t                      link_hdr_len;
};

struct pcap_packet {
	struct packet core_packet;
	struct packet_module_state *state;
	struct pcap_pkthdr header;
	u_char *data;
};


/* Init parameters */
static char *input_file;
static char *input_iface;
static char *output_file;


static void cleanup()
{
	free(input_file);
	free(input_iface);
	free(output_file);
}

static int init(int argc, char *argv[])
{
	int index = 0;

	if (argc < 2) {
		messagef(HAKA_LOG_ERROR, L"pcap", L"specify a device (-i) or a pcap filename (-f)");
		cleanup();
		return 1;
	}

	/* get a pcap descriptor from device */
	if (strcmp(argv[0], "-i") == 0) {
		input_iface = strdup(argv[1]);
	}
	/* get a pcap descriptor from a pcap file */
	else if (strcmp(argv[0], "-f") == 0) {
		input_file = strdup(argv[1]);
	}
	/* unkonwn options */
	else  {
		messagef(HAKA_LOG_ERROR, L"pcap", L"unkown options");
		cleanup();
		return 1;
	}

	index += 2;

	while (argc-index >= 1) {
		if (strcmp(argv[index], "-o") == 0) {
			if (argc-index >= 2) {
				output_file = strdup(argv[++index]);
			}
			else {
				messagef(HAKA_LOG_ERROR, L"pcap", L"-o must be followed by the file output name");
				cleanup();
				return 1;
			}
		}
		else {
			messagef(HAKA_LOG_ERROR, L"pcap", L"unknown option '%s'", argv[index]);
			cleanup();
			return 1;
		}

		++index;
	}

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
	int linktype;
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
	if ((linktype = pcap_datalink(state->pd)) < 0)
	{
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(state->pd));
		cleanup_state(state);
		return NULL;
	}

	/* Set the datalink layer header size. */
	switch (linktype)
	{
	case DLT_NULL:
		state->link_hdr_len = 4;
		break;

	case DLT_LINUX_SLL:
		state->link_hdr_len = 16;
		break;

	case DLT_EN10MB:
		state->link_hdr_len = 14;
		break;

	case DLT_SLIP:
	case DLT_PPP:
		state->link_hdr_len = 24;
		break;

	default:
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", "unsupported data link");
		cleanup_state(state);
		return NULL;
	}

	return state;
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	struct pcap_pkthdr *header;
	const u_char *p;
	int ret;

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

		packet->data = malloc(header->caplen);
		if (!packet->data) {
			free(packet);
			return ENOMEM;
		}

		/* fill packet data structure */
		memcpy(packet->data, p, header->caplen);
		packet->header = *header;
		packet->state = state;

		if (packet->header.caplen < packet->header.len)
			messagef(HAKA_LOG_WARNING, L"pcap", L"packet truncated");

		*pkt = (struct packet *)packet;
		return 0;
	}
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	/* dump capture in pcap file */
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	if (pkt->state->pf && result == FILTER_ACCEPT)
		pcap_dump((u_char *)pkt->state->pf, &(pkt->header), pkt->data);

	free(pkt->data);
	free(pkt);
}

static size_t packet_get_length(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return pkt->header.caplen - pkt->state->link_hdr_len;
}

static const uint8 *packet_get_data(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return (pkt->data + pkt->state->link_hdr_len);
}

static uint8 *packet_modifiable(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return (pkt->data + pkt->state->link_hdr_len);
}

static int packet_do_resize(struct packet *orig_pkt, size_t size)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	u_char *new_data;
	const size_t new_size = size + pkt->state->link_hdr_len;
	const size_t copy_size = MIN(new_size, pkt->header.caplen);

	new_data = malloc(new_size);
	if (!new_data) {
		return ENOMEM;
	}

	memcpy(new_data, pkt->data, copy_size);

	free(pkt->data);
	pkt->data = new_data;
	pkt->header.len = new_size;
	pkt->header.caplen = new_size;
	return 0;
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return (pkt->header.ts.tv_sec << 32) + pkt->header.ts.tv_usec;
}

static const char *packet_get_dissector(struct packet *pkt)
{
	return "ipv4";
}


/**
 * @defgroup Pcap Pcap
 * @brief Packet capturing using the libpcap.
 * @author Arkoon Network Security
 * @ingroup ExternPacketModule
 *
 * # Description
 *
 * The module uses the library pcap to read packets from a file or from an real network
 * device.
 *
 * To be able to capture packets on a real interface, the process need to be launched
 * with the proper access rights.
 *
 * # Initialization arguments
 *
 * ~~~~~~~~
 * ( -f <pcap file> | -i <interface name> ̀| -i any ) [ -o <pcap file> ] [-m]
 * ~~~~~~~~
 *
 * # Usage
 *
 * Module usage.
 *
 * ### Lua
 * ~~~~~~~~{.lua}
 * app.install("packet", module.load("packet-pcap", {"-f", "dump.pcap", "-o", "output.pcap"}))
 * ~~~~~~~~
 */
struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        L"Pcap Module",
		description: L"Pcap packet module",
		author:      L"Arkoon Network Security",
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
	get_dissector:   packet_get_dissector
};

