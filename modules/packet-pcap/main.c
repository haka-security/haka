
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

struct packet_module_state;

struct packet_module_unique_state {
	pcap_t                     *pd;
	pcap_dumper_t              *pf;
	size_t                      link_hdr_len;
	atomic_t                    count;
	mutex_t                     mutex;
	struct packet_module_state *first;
};

struct packet_module_state {
	struct packet_module_unique_state *unique_state;
	semaphore_t                        wait;
	struct packet_module_state        *next;
	struct packet_module_state        *prev;
};

struct packet {
	struct packet_module_state *state;
	struct pcap_pkthdr header;
	u_char data[0];
};


/* Init parameters */
static char *input_file;
static char *input_iface;
static char *output_file;

/*
 * The pcap module supports multi-threading for testing purposes. In this mode
 * the threads are not running concurrently but only one is active at a time
 * and will receive a packet. The next packet will then be handled by another
 * thread.
 */
static bool multi_threaded_pcap = false;
static struct packet_module_unique_state *unique_state = NULL;


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
		else if (strcmp(argv[index], "-m") == 0) {
			multi_threaded_pcap = true;
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
	return multi_threaded_pcap;
}

static void cleanup_unique_state()
{
	if (unique_state->pf) {
		pcap_dump_close(unique_state->pf);
		unique_state->pf = NULL;
	}

	if (unique_state->pd) {
		pcap_close(unique_state->pd);
		unique_state->pd = NULL;
	}

	mutex_destroy(&unique_state->mutex);

	free(unique_state);
	unique_state = NULL;
}

static void cleanup_state(struct packet_module_state *state)
{
	if (atomic_dec(&state->unique_state->count) == 0) {
		cleanup_unique_state();
	}

	semaphore_destroy(&state->wait);
	free(state);
}

static bool init_unique_state()
{
	int linktype;
	char errbuf[PCAP_ERRBUF_SIZE];

	unique_state = malloc(sizeof(struct packet_module_unique_state));
	if (!unique_state) {
		return false;
	}

	bzero(unique_state, sizeof(struct packet_module_state));
	bzero(errbuf, PCAP_ERRBUF_SIZE);

	mutex_init(&unique_state->mutex);

	/* get a pcap descriptor from device */
	if (input_iface) {
		unique_state->pd = pcap_open_live(input_iface, SNAPLEN, 1, 0, errbuf);
		if (unique_state->pd && (strlen(errbuf) > 0)) {
			messagef(HAKA_LOG_WARNING, L"pcap", L"%s", errbuf);
		}
	}
	/* get a pcap descriptor from a pcap file */
	else if (input_file) {
		unique_state->pd = pcap_open_offline(input_file, errbuf);
	}
	else {
		/* This case should never be reached */
		assert(0);
	}

	if (!unique_state->pd) {
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errbuf);
		cleanup_unique_state(unique_state);
		return false;
	}

	if (output_file) {
		/* open pcap savefile */
		unique_state->pf = pcap_dump_open(unique_state->pd, output_file);
		if (!unique_state->pf) {
			cleanup_unique_state(unique_state);
			messagef(HAKA_LOG_ERROR, L"pcap", L"unable to dump on %s", output_file);
			return false;
		}
	}

	/* Determine the datalink layer type. */
	if ((linktype = pcap_datalink(unique_state->pd)) < 0)
	{
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(unique_state->pd));
		cleanup_unique_state(unique_state);
		return false;
	}

	/* Set the datalink layer header size. */
	switch (linktype)
	{
	case DLT_NULL:
		unique_state->link_hdr_len = 4;
		break;

	case DLT_LINUX_SLL:
		unique_state->link_hdr_len = 16;
		break;

	case DLT_EN10MB:
		unique_state->link_hdr_len = 14;
		break;

	case DLT_SLIP:
	case DLT_PPP:
		unique_state->link_hdr_len = 24;
		break;

	default:
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", "unsupported data link");
		cleanup_unique_state(unique_state);
		return false;
	}

	return true;
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;

	if (!unique_state) {
		if (!init_unique_state()) {
			return NULL;
		}
	}

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		return NULL;
	}

	memset(state, 0, sizeof(struct packet_module_state));

	state->unique_state = unique_state;
	atomic_inc(&unique_state->count);
	semaphore_init(&state->wait, 0);

	return state;
}

/*
 * The semaphore does not schedule the waiting thread the way we want it. It
 * is not FIFO. We then have to redo the scheduling ourself to make sure each
 * packet will be handled by a different thread.
 */
static void wait(struct packet_module_state *state)
{
	if (multi_threaded_pcap) {
		bool should_wait;

		mutex_lock(&state->unique_state->mutex);

		should_wait = (state->unique_state->first != NULL);

		assert(state->next == NULL);
		assert(state->prev == NULL);

		state->next = state->unique_state->first;
		if (state->next) {
			state->prev = state->next->prev;
			state->next->prev = state;
		}
		state->unique_state->first = state;

		mutex_unlock(&state->unique_state->mutex);

		if (should_wait)
			semaphore_wait(&state->wait);
	}
}

static void post(struct packet_module_state *state)
{
	if (multi_threaded_pcap) {
		struct packet_module_state *next = NULL;

		mutex_lock(&state->unique_state->mutex);

		next = state->prev;

		if (state->unique_state->first == state) {
			state->unique_state->first = state->next;
		}

		if (state->next) state->next->prev = state->prev;
		if (state->prev) state->prev->next = state->next;
		state->next = NULL;
		state->prev = NULL;

		mutex_unlock(&state->unique_state->mutex);

		if (next) {
			semaphore_post(&next->wait);
		}
	}
}

static int packet_receive(struct packet_module_state *state, struct packet **pkt)
{
	struct pcap_pkthdr *header;
	struct packet *packet = NULL;
	const u_char *p;
	int ret;

	wait(state);

	/* read packet */
	ret = pcap_next_ex(state->unique_state->pd, &header, &p);

	if(ret == -1) {
		/* error while reading packet */
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(state->unique_state->pd));
		post(state);
		return 1;
	}
	else if (ret == -2) {
		/* end of pcap file */
		post(state);
		return 1;
	}
	else if (ret == 0) {
		/* Timeout expired. */
		post(state);
		return 0;
	}
	else {
		packet = malloc(sizeof(struct packet) + header->caplen);
		if (!packet) {
			post(state);
			return ENOMEM;
		}

		/* fill packet data structure */
		memcpy(packet->data, p, header->caplen);
		packet->header = *header;
		packet->state = state;

		if (packet->header.caplen < packet->header.len)
			messagef(HAKA_LOG_WARNING, L"pcap", L"packet truncated");

		*pkt = packet;
		return 0;
	}
}

static void packet_verdict(struct packet *pkt, filter_result result)
{
	/* dump capture in pcap file */
	if (pkt->state->unique_state->pf && result == FILTER_ACCEPT)
		pcap_dump((u_char *)pkt->state->unique_state->pf, &(pkt->header), pkt->data);

	post(pkt->state);
	free(pkt);
}

static size_t packet_get_length(struct packet *pkt)
{
	return pkt->header.caplen - pkt->state->unique_state->link_hdr_len;
}

static const uint8 *packet_get_data(struct packet *pkt)
{
	return (pkt->data + pkt->state->unique_state->link_hdr_len);
}

static uint8 *packet_make_modifiable(struct packet *pkt)
{
	return (pkt->data + pkt->state->unique_state->link_hdr_len);
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
	receive:         packet_receive,
	verdict:         packet_verdict,
	get_length:      packet_get_length,
	make_modifiable: packet_make_modifiable,
	get_data:        packet_get_data
};

