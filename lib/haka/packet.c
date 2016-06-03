/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <haka/packet.h>
#include <haka/vbuffer.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/timer.h>
#include <haka/capture_module.h>
#include <haka/engine.h>


static struct capture_module *capture_module = NULL;
static enum capture_mode global_capture_mode = MODE_NORMAL;
static local_storage_t capture_state;
struct time_realm network_time;
static bool is_realtime = false;
static bool network_time_inited = false;

INIT static void _init()
{
	UNUSED const bool ret = local_storage_init(&capture_state, NULL);
	assert(ret);
}

FINI static void _fini()
{
	UNUSED bool ret = local_storage_destroy(&capture_state);
	assert(ret);

	if (network_time_inited) {
		ret = time_realm_destroy(&network_time);
		assert(ret);
	}
}

int set_capture_module(struct module *module)
{
	struct capture_module *prev_capture_module = capture_module;

	if (module && module->type != MODULE_CAPTURE) {
		error("'%s' is not a packet capture module", module->name);
		return 1;
	}

	if (module) {
		capture_module = (struct capture_module *)module;
		module_addref(&capture_module->module);
	}
	else {
		capture_module = NULL;
	}

	if (prev_capture_module) {
		module_release(&prev_capture_module->module);
	}

	if (network_time_inited) {
		time_realm_destroy(&network_time);
		network_time_inited = false;
	}

	if (capture_module) {
		time_realm_initialize(&network_time,
				capture_module->is_realtime() ? TIME_REALM_REALTIME : TIME_REALM_STATIC);
		network_time_inited = true;
		is_realtime = capture_module->is_realtime();
	}

	return 0;
}

int has_capture_module()
{
	return capture_module != NULL;
}

struct capture_module *get_capture_module()
{
	return capture_module;
}

bool packet_init(struct capture_module_state *state)
{
	assert(!local_storage_get(&capture_state));
	return local_storage_set(&capture_state, state);
}

static struct capture_module_state *get_capture_state()
{
	struct capture_module_state *state = local_storage_get(&capture_state);
	assert(state);
	return state;
}

const char *packet_dissector(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	return capture_module->get_dissector(pkt);
}

struct vbuffer *packet_payload(struct packet *pkt)
{
	assert(vbuffer_isvalid(&pkt->payload));
	return &pkt->payload;
}

int packet_receive(struct engine_thread *engine, struct packet **pkt)
{
	int ret;
	assert(capture_module);

	ret = capture_module->receive(get_capture_state(), pkt);

	if (!ret && *pkt) {
		(*pkt)->lua_object = lua_object_init;
		lua_ref_init(&(*pkt)->userdata);
		lua_ref_init(&(*pkt)->next_dissector);
		atomic_set(&(*pkt)->ref, 1);
		assert(vbuffer_isvalid(&(*pkt)->payload));
		LOG_DEBUG(packet, "received packet id=%lli",
				capture_module->get_id(*pkt));

		{
			volatile struct packet_stats *stats = engine_thread_statistics(engine);
			if (stats) {
				++stats->recv_packets;
				stats->recv_bytes += vbuffer_size(packet_payload(*pkt));
			}
		}
	}

	if (*pkt && !is_realtime) {
		time_realm_update_and_check(&network_time,
					capture_module->get_timestamp(*pkt));
	}
	else {
		time_realm_check(&network_time);
	}

	return ret;
}

void packet_drop(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	LOG_DEBUG(packet, "dropping packet id=%lli",
			capture_module->get_id(pkt));

	capture_module->verdict(pkt, FILTER_DROP);

	{
		volatile struct packet_stats *stats = engine_thread_statistics(engine_thread_current());
		if (stats) ++stats->drop_packets;
	}
}

void packet_accept(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);

	LOG_DEBUG(packet, "accepting packet id=%lli",
			capture_module->get_id(pkt));

	{
		volatile struct packet_stats *stats = engine_thread_statistics(engine_thread_current());
		if (stats) {
			++stats->trans_packets;
			stats->trans_bytes += vbuffer_size(packet_payload(pkt));
		}
	}

	capture_module->verdict(pkt, FILTER_ACCEPT);
}

void packet_addref(struct packet *pkt)
{
	assert(pkt);
	atomic_inc(&pkt->ref);
}

bool packet_release(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	if (atomic_dec(&pkt->ref) == 0) {
		lua_ref_clear(&pkt->userdata);
		lua_ref_clear(&pkt->next_dissector);
		lua_object_release(pkt, &pkt->lua_object);
		capture_module->release_packet(pkt);
		return true;
	}
	else {
		return false;
	}
}

struct packet *packet_new(size_t size)
{
	struct packet *pkt;

	assert(capture_module);

	pkt = capture_module->new_packet(get_capture_state(), size);
	if (!pkt) {
		assert(check_error());
		return NULL;
	}

	pkt->lua_object = lua_object_init;
	lua_ref_init(&pkt->userdata);
	lua_ref_init(&pkt->next_dissector);
	atomic_set(&pkt->ref, 1);
	assert(vbuffer_isvalid(&pkt->payload));

	return pkt;
}

bool packet_send(struct packet *pkt)
{
	assert(pkt);
	assert(capture_module);

	switch (packet_state(pkt)) {
	case STATUS_FORGED:
		break;

	case STATUS_NORMAL:
	case STATUS_SENT:
		error("operation not supported (packet captured)");
		return false;

	default:
		assert(0);
		return false;
	}

	LOG_DEBUG(packet, "sending packet id=%lli",
		capture_module->get_id(pkt));

	{
		volatile struct packet_stats *stats = engine_thread_statistics(engine_thread_current());
		if (stats) {
			++stats->trans_packets;
			stats->trans_bytes += vbuffer_size(packet_payload(pkt));
		}
	}

	return capture_module->send_packet(pkt);
}

enum packet_status packet_state(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	return capture_module->packet_getstate(pkt);
}

size_t packet_mtu(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	return capture_module->get_mtu(pkt);
}

const struct time *packet_timestamp(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	return capture_module->get_timestamp(pkt);
}

uint64 packet_id(struct packet *pkt)
{
	assert(capture_module);
	assert(pkt);
	return capture_module->get_id(pkt);
}

void capture_set_mode(enum capture_mode mode)
{
	global_capture_mode = mode;
}

enum capture_mode capture_mode()
{
	return global_capture_mode;
}
