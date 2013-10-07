
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

struct pcap_capture {
	pcap_t       *pd;
	int           link_type;
};

struct packet_module_state {
	uint32                      pd_count;
	struct pcap_capture        *pd;
	pcap_dumper_t              *pf;
	uint64                      packet_id;
	int                         link_type;
	struct pcap_packet         *sent_head;
	struct pcap_packet         *sent_tail;
};

struct pcap_packet {
	struct packet               core_packet;
	struct list                 list;
	struct time                 timestamp;
	struct packet_module_state *state;
	struct pcap_pkthdr          header;
	u_char                     *data;
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
static int    input_count;
static char **inputs;
static bool   input_is_iface;
static char  *output_file;


static void cleanup()
{
	int i;

	for (i=0; i<input_count; ++i) {
		free(inputs[i]);
	}

	free(inputs);
	free(output_file);
}

static int init(struct parameters *args)
{
	const char *input, *output, *interfaces;

	interfaces = parameters_get_string(args, "interfaces", NULL);
	input = parameters_get_string(args, "file", NULL);

	if (interfaces) {
		int count, i;
		const char *iter;
		char *interfaces_buf, *str, *ptr = NULL;

		for (count = 0, iter = interfaces; *iter; ++iter) {
			if (*iter == ',')
				++count;
		}

		++count;

		interfaces_buf = strdup(interfaces);
		if (!interfaces_buf) {
			error(L"memory error");
			cleanup();
			return 1;
		}

		input_count = count;
		inputs = malloc(sizeof(char *)*count);
		if (!inputs) {
			free(interfaces_buf);
			error(L"memory error");
			cleanup();
			return 1;
		}

		for (i = 0, str = interfaces_buf; i < count; i++, str=NULL) {
			char *token = strtok_r(str, ",", &ptr);
			assert(token != NULL);
			inputs[i] = strdup(token);
			if (!inputs[i]) {
				free(interfaces_buf);
				cleanup();
				return 1;
			}
		}

		free(interfaces_buf);

		input_is_iface = true;
	}
	else if (input) {
		input_count = 1;
		inputs = malloc(sizeof(char *));
		if (!inputs) {
			error(L"memory error");
			cleanup();
			return 1;
		}

		inputs[0] = strdup(input);
		if (!inputs[0]) {
			error(L"memory error");
			cleanup();
			return 1;
		}

		input_is_iface = false;
	}
	else {
		messagef(HAKA_LOG_ERROR, L"pcap", L"specifiy either a device or a pcap filename");
		cleanup();
		return 1;
	}

	if ((output = parameters_get_string(args, "output", NULL))) {
		output_file = strdup(output);
	}

	return 0;
}

static bool multi_threaded()
{
	return false;
}

static void cleanup_state(struct packet_module_state *state)
{
	int i;

	if (state->pf) {
		pcap_dump_close(state->pf);
		state->pf = NULL;
	}

	for (i=0; i<state->pd_count; ++i) {
		if (state->pd[i].pd) {
			pcap_close(state->pd[i].pd);
			state->pd[i].pd = NULL;
		}
	}

	free(state->pd);
	free(state);
}

static bool open_pcap(struct pcap_capture *pd, const char *input, bool isiface)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	bzero(errbuf, PCAP_ERRBUF_SIZE);

	assert(input);

	if (isiface) {
		messagef(HAKA_LOG_INFO, L"pcap", L"listening on device %s", input);

		pd->pd = pcap_open_live(input, SNAPLEN, 1, 0, errbuf);
		if (pd->pd && (strlen(errbuf) > 0)) {
			messagef(HAKA_LOG_WARNING, L"pcap", L"%s", errbuf);
		}
	}
	else {
		messagef(HAKA_LOG_INFO, L"pcap", L"openning file '%s'", input);

		pd->pd = pcap_open_offline(input, errbuf);
	}

	if (!pd->pd) {
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errbuf);
		return false;
	}

	/* Determine the datalink layer type. */
	if ((pd->link_type = pcap_datalink(pd->pd)) < 0)
	{
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(pd->pd));
		return false;
	}

	/* Check for supported datalink layer. */
	switch (pd->link_type)
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
		return false;
	}

	return true;
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;
	int i;

	assert(input_count > 0);

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		error(L"memory error");
		return NULL;
	}

	bzero(state, sizeof(struct packet_module_state));

	state->pd = malloc(sizeof(struct pcap_capture)*input_count);
	if (!state->pd) {
		error(L"memory error");
		free(state);
		return NULL;
	}

	state->pd_count = input_count;
	bzero(state->pd, sizeof(struct pcap_capture)*input_count);

	for (i=0; i<input_count; ++i) {
		if (!open_pcap(&state->pd[i], inputs[i], input_is_iface)) {
			cleanup_state(state);
			return NULL;
		}
	}

	state->link_type = state->pd[0].link_type;

	if (output_file) {
		/* open pcap savefile */
		state->pf = pcap_dump_open(state->pd[0].pd, output_file);
		if (!state->pf) {
			cleanup_state(state);
			messagef(HAKA_LOG_ERROR, L"pcap", L"unable to dump on %s", output_file);
			return NULL;
		}
	}

	state->packet_id = 0;

	return state;
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
		int i;
		int ret;
		fd_set read_set;
		int max_fd = -1;

		/* read packet */
		FD_ZERO(&read_set);

		for (i=0; i<state->pd_count; ++i) {
			const int fd = pcap_get_selectable_fd(state->pd[i].pd);
			if (fd < 0) {
				messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errno_error(errno));
				return 1;
			}
			else {
				FD_SET(fd, &read_set);
				if (fd > max_fd) max_fd = fd;
			}
		}

		ret = select(max_fd+1, &read_set, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR) {
				return 0;
			}
			else {
				messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errno_error(errno));
				return 1;
			}
		}
		else if (ret == 0) {
			return 0;
		}
		else {
			struct pcap_capture *pd = NULL;
			struct pcap_pkthdr *header;
			const u_char *p;

			for (i=0; i<state->pd_count; ++i) {
				const int fd = pcap_get_selectable_fd(state->pd[i].pd);
				if (FD_ISSET(fd, &read_set)) {
					pd = &state->pd[i];
					break;
				}
			}

			assert(pd);

			ret = pcap_next_ex(pd->pd, &header, &p);
			if (ret == -1) {
				messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(pd->pd));
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
				packet->link_type = pd->link_type;
				packet->id = state->packet_id++;
				packet->timestamp.secs = header->ts.tv_sec;
				packet->timestamp.nsecs = header->ts.tv_usec*1000;

				if (packet->header.caplen < packet->header.len)
					messagef(HAKA_LOG_WARNING, L"pcap", L"packet truncated");

				*pkt = (struct packet *)packet;
				return 0;
			}
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
	switch (pkt->link_type)
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
	packet->link_type = state->link_type;
	time_gettimestamp(&packet->timestamp);

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

static const struct time *get_timestamp(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;
	return &pkt->timestamp;
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
