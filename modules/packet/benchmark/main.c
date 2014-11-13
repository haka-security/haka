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

static REGISTER_LOG_SECTION(benchmark);

#define PROGRESS_DELAY      5 /* 5 seconds */
#define PROGRESS_FREQ       10000
#define MEBI 1048576.f

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
	struct pcap_capture  pd;
	uint64               packet_id;
	struct pcap_packet  *received_head;
	struct pcap_packet  *current;
	struct pcap_packet  *received_tail;
	int                  repeated;
	size_t               size;
	uint64               packet_count;
	uint64               progress_samples;
	bool                 started;
	struct time          start;
	struct time          end;
};

/* Init parameters */
static char         *input_file;
static bool          passthrough = true;
static int           repeat = 1;
static mutex_t       stats_lock = MUTEX_INIT;
static struct time   start = INVALID_TIME;
static struct time   end = INVALID_TIME;
static size_t        size;
static uint64        packet_count;

static void cleanup()
{
	struct time difftime;
	double duration;
	double bandwidth;
	double packets_per_s;

	time_diff(&difftime, &end, &start);
	duration = time_sec(&difftime);
	bandwidth = size * 8 / duration / MEBI;
	packets_per_s = packet_count / duration;

	LOG_INFO(benchmark,
			"processing %zd bytes in %llu packets took %lld.%.9u seconds being %02f Mib/s and %02f packets/s",
			size, packet_count, (int64)difftime.secs, difftime.nsecs, bandwidth, packets_per_s);

	free(input_file);
}

static int init(struct parameters *args)
{
	const char *input;

	if ((input = parameters_get_string(args, "file", NULL))) {
		input_file = strdup(input);
	}
	else {
		LOG_ERROR(benchmark, "missing input parameter");
		cleanup();
		return 1;
	}

	passthrough = parameters_get_boolean(args, "pass-through", true);
	repeat = parameters_get_integer(args, "repeat", 1);

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
	mutex_lock(&stats_lock);
	if (!time_isvalid(&start) || time_cmp(&state->start, &start) < 0) {
		start = state->start;
	}

	time_gettimestamp(&state->end);
	if (!time_isvalid(&end) || time_cmp(&state->end, &end) > 0) {
		end = state->end;
	}
	size += state->size * state->repeated;
	packet_count += state->packet_count * state->repeated;
	mutex_unlock(&stats_lock);

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
		LOG_ERROR(benchmark, "%s", pcap_geterr(state->pd.pd));
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
		LOG_ERROR(benchmark, "skipping malformed packet %llu", ++state->packet_id);
		return 0;
	}
	else {
		struct pcap_packet *packet = malloc(sizeof(struct pcap_packet));
		if (!packet) {
			return ENOMEM;
		}

		memset(packet, 0, sizeof(struct pcap_packet));
		packet_init(&packet->core_packet);

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
			LOG_WARNING(benchmark, "packet truncated");

		packet->protocol = get_protocol(state->pd.link_type, &data, &data_offset);

		vbuffer_sub_create(&sub, &data, data_offset, ALL);
		ret = vbuffer_select(&sub, &packet->core_packet.payload, NULL);
		vbuffer_release(&data);
		if (!ret) {
			LOG_ERROR(benchmark, "malformed packet %llu", packet->id);
			free(packet);
			return ENOMEM;
		}

		state->size += header->caplen - data_offset;
		state->packet_count++;

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

	LOG_INFO(benchmark, "opening file '%s'", input);

	state->pd.pd = pcap_open_offline(input, errbuf);

	if (!state->pd.pd) {
		LOG_ERROR(benchmark, "%s", errbuf);
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
		LOG_ERROR(benchmark, "%s", pcap_geterr(state->pd.pd));
		pcap_close(state->pd.pd);
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
		LOG_ERROR(benchmark, "%s", "unsupported data link");
		pcap_close(state->pd.pd);
		return false;
	}

	LOG_INFO(benchmark, "loading packet in memory from '%s'", input);
	while(load_packet(state) == 0);
	LOG_INFO(benchmark, "loaded %zd bytes in memory", state->size);

	pcap_close(state->pd.pd);

	return true;
}

static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		error("memory error");
		return NULL;
	}

	state->packet_id = 0;
	state->repeated = 0;
	state->size = 0;
	state->packet_count = 0;
	state->progress_samples = 0;
	state->started = false;

	bzero(state, sizeof(struct packet_module_state));

	if (!load_pcap(state, input_file)) {
		cleanup_state(state);
		return NULL;
	}

	return state;
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
	if (!state->started) {
		time_gettimestamp(&state->start);
		state->started = true;
	}

	if (!state->current) {
		state->repeated++;
		if (state->repeated < repeat) {
			if (state->progress_samples > PROGRESS_FREQ) {
				const float percent = state->repeated * 100.f / repeat;
				struct time time, difftime;
				time_gettimestamp(&time);

				if (time_isvalid(&state->pd.last_progress)) {
					time_diff(&difftime, &time, &state->pd.last_progress);

					if (difftime.secs >= PROGRESS_DELAY) {
						state->pd.last_progress = time;
						if (percent > 0) {
							LOG_INFO(benchmark, "progress %.2f %%", percent);
						}
					}
				} else {
					state->pd.last_progress = time;
				}

				state->progress_samples %= PROGRESS_FREQ;
			}

			state->current = state->received_head;
			state->progress_samples += state->packet_count;
		} else {
			/* No more packet */
			return 1;
		}
	}

	*pkt = &state->current->core_packet;
	lua_object_release(&(*pkt)->lua_object);
	lua_ref_clear(NULL, &(*pkt)->luadata);

	state->current = list_next(state->current);
	return 0;
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	struct pcap_packet *pkt = (struct pcap_packet*)orig_pkt;

	switch (pkt->protocol) {
	case ETH_P_IP:
		return "ipv4";

	case -1:
		LOG_ERROR(benchmark, "malformed packet %llu", pkt->id);

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
		error("Memory error");
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
	error("sending is not supported");
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
		name:        "Benchmark Module",
		description: "Packet capture from memory module",
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
