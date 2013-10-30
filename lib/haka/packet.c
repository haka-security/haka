
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <haka/packet.h>
#include <haka/vbuffer.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/packet_module.h>


static struct packet_module *packet_module = NULL;
static enum packet_mode global_packet_mode = MODE_NORMAL;
static local_storage_t capture_state;

INIT static void __init()
{
	UNUSED const bool ret = local_storage_init(&capture_state, NULL);
	assert(ret);
}

FINI static void __fini()
{
	UNUSED const bool ret = local_storage_destroy(&capture_state);
	assert(ret);
}

int set_packet_module(struct module *module)
{
	struct packet_module *prev_packet_module = packet_module;

	if (module && module->type != MODULE_PACKET) {
		error(L"'%ls' is not a packet module", module->name);
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
	assert(pkt->payload);
	return pkt->payload;
}

int packet_receive(struct packet **pkt)
{
	int ret;
	assert(packet_module);

	ret = packet_module->receive(get_capture_state(), pkt);
	if (!ret && *pkt) {
		lua_object_init(&(*pkt)->lua_object);
		atomic_set(&(*pkt)->ref, 1);
		assert((*pkt)->payload);
		messagef(HAKA_LOG_DEBUG, L"packet", L"received packet id=%lli",
				packet_module->get_id(*pkt));
	}

	return ret;
}

void packet_drop(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	messagef(HAKA_LOG_DEBUG, L"packet", L"dropping packet id=%lli",
			packet_module->get_id(pkt));

	packet_module->verdict(pkt, FILTER_DROP);
}

void packet_accept(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);

	messagef(HAKA_LOG_DEBUG, L"packet", L"accepting packet id=%lli",
			packet_module->get_id(pkt));

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
		if (pkt->payload) {
			vbuffer_free(pkt->payload);
			pkt->payload = NULL;
		}
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

	lua_object_init(&pkt->lua_object);
	atomic_set(&pkt->ref, 1);
	assert(pkt->payload);

	return pkt;
}

bool packet_send(struct packet *pkt)
{
	assert(pkt);
	assert(packet_module);

	switch (packet_state(pkt)) {
	case STATUS_FORGED:
		break;

	case STATUS_NORMAL:
	case STATUS_SENT:
		error(L"operation not supported (packet captured)");
		return false;

	default:
		assert(0);
		return false;
	}

	messagef(HAKA_LOG_DEBUG, L"packet", L"sending packet id=%lli",
		packet_module->get_id(pkt));

	return packet_module->send_packet(pkt);
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

void packet_set_mode(enum packet_mode mode)
{
	global_packet_mode = mode;
}

enum packet_mode packet_mode()
{
	return global_packet_mode;
}
