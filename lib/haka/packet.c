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
#include <haka/packet_module.h>
#include <haka/engine.h>


static struct packet_module *packet_module = NULL;
static enum packet_mode global_packet_mode = MODE_NORMAL;
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

int set_packet_module(struct module *module)
{
	struct packet_module *prev_packet_module = packet_module;

	if (module && module->type != MODULE_PACKET) {
		error("'%s' is not a packet module", module->name);
		return 1;
	}

	if (module) {
		packet_module = (struct packet_module *)module;
		module_addref(&packet_module->module);
	}
	else {
		packet_module = NULL;
	}

	if (prev_packet_module) {
		module_release(&prev_packet_module->module);
	}

	if (network_time_inited) {
		time_realm_destroy(&network_time);
		network_time_inited = false;
	}

	if (packet_module) {
		time_realm_initialize(&network_time,
				packet_module->is_realtime() ? TIME_REALM_REALTIME : TIME_REALM_STATIC);
		network_time_inited = true;
		is_realtime = packet_module->is_realtime();
	}

	return 0;
}

int has_packet_module()
{
	return packet_module != NULL;
}

struct packet_module *get_packet_module()
{
	return packet_module;
}

bool packet_init(struct packet_module_state *state)
{
	assert(!local_storage_get(&capture_state));
	return local_storage_set(&capture_state, state);
}

static struct packet_module_state *get_capture_state()
{
	struct packet_module_state *state = local_storage_get(&capture_state);
	assert(state);
	return state;
}

const char *packet_dissector(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_dissector(pkt);
}

struct vbuffer *packet_payload(struct packet *pkt)
{
	assert(vbuffer_isvalid(&pkt->payload));
	return &pkt->payload;
}

int packet_receive(struct engine_thread *engine, struct packet **pkt)
{
	int ret;
	assert(packet_module);

	ret = packet_module->receive(get_capture_state(), pkt);

	if (!ret && *pkt) {
		(*pkt)->lua_object = lua_object_init;
		lua_ref_init(&(*pkt)->userdata);
		atomic_set(&(*pkt)->ref, 1);
		assert(vbuffer_isvalid(&(*pkt)->payload));
		LOG_DEBUG(packet, "received packet id=%lli",
				packet_module->get_id(*pkt));

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
					packet_module->get_timestamp(*pkt));
	}
	else {
		time_realm_check(&network_time);
	}

	return ret;
}

void packet_drop(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	LOG_DEBUG(packet, "dropping packet id=%lli",
			packet_module->get_id(pkt));

	packet_module->verdict(pkt, FILTER_DROP);

	{
		volatile struct packet_stats *stats = engine_thread_statistics(engine_thread_current());
		if (stats) ++stats->drop_packets;
	}
}

void packet_send(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);

	switch (packet_state(pkt)) {
		case STATUS_FORGED:
		case STATUS_NORMAL:
			break;

		case STATUS_SENT:
			error("operation not supported");
			return;

		default:
			assert(0);
			return;
	}

	LOG_DEBUG(packet, "accepting packet id=%lli",
			packet_module->get_id(pkt));

	{
		volatile struct packet_stats *stats = engine_thread_statistics(engine_thread_current());
		if (stats) {
			++stats->trans_packets;
			stats->trans_bytes += vbuffer_size(packet_payload(pkt));
		}
	}

	packet_module->verdict(pkt, FILTER_ACCEPT);
}

void packet_addref(struct packet *pkt)
{
	assert(pkt);
	atomic_inc(&pkt->ref);
}

bool packet_release(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	if (atomic_dec(&pkt->ref) == 0) {
		lua_ref_clear(&pkt->userdata);
		lua_object_release(pkt, &pkt->lua_object);
		packet_module->release_packet(pkt);
		return true;
	}
	else {
		return false;
	}
}

struct packet *packet_new(size_t size)
{
	struct packet *pkt;

	assert(packet_module);

	pkt = packet_module->new_packet(get_capture_state(), size);
	if (!pkt) {
		assert(check_error());
		return NULL;
	}

	pkt->lua_object = lua_object_init;
	lua_ref_init(&pkt->userdata);
	atomic_set(&pkt->ref, 1);
	assert(vbuffer_isvalid(&pkt->payload));

	return pkt;
}

void packet_inject(struct packet *pkt)
{
	assert(pkt);
	assert(packet_module);

	switch (packet_state(pkt)) {
	case STATUS_FORGED:
		break;

	case STATUS_NORMAL:
	case STATUS_SENT:
		error("operation not supported (packet captured)");
		return;

	default:
		assert(0);
		return;
	}

	LOG_DEBUG(packet, "sending packet id=%lli",
		packet_module->get_id(pkt));

	{
		volatile struct packet_stats *stats = engine_thread_statistics(engine_thread_current());
		if (stats) {
			++stats->trans_packets;
			stats->trans_bytes += vbuffer_size(packet_payload(pkt));
		}
	}

	packet_module->send_packet(pkt);
}

enum packet_status packet_state(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->packet_getstate(pkt);
}

size_t packet_mtu(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_mtu(pkt);
}

const struct time *packet_timestamp(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_timestamp(pkt);
}

uint64 packet_id(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_id(pkt);
}

void packet_set_mode(enum packet_mode mode)
{
	global_packet_mode = mode;
}

struct packet *packet_from_userdata(void *pkt)
{
	return (struct packet *)pkt;
}

enum packet_mode packet_mode()
{
	return global_packet_mode;
}
