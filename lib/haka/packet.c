
#include <assert.h>
#include <stdlib.h>

#include <haka/packet.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/packet_module.h>


static struct packet_module *packet_module = NULL;
static enum packet_mode global_packet_mode = MODE_NORMAL;

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

size_t packet_length(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_length(pkt);
}

const uint8 *packet_data(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_data(pkt);
}

const char *packet_dissector(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->get_dissector(pkt);
}

uint8 *packet_data_modifiable(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);

	switch (global_packet_mode) {
	case MODE_NORMAL:
		return packet_module->make_modifiable(pkt);

	case MODE_PASSTHROUGH:
		error(L"operation not supported (pass-through mode)");
		return NULL;

	default:
		assert(0);
		return NULL;
	}
}

int packet_resize(struct packet *pkt, size_t size)
{
	assert(packet_module);

	switch (global_packet_mode) {
	case MODE_NORMAL:
		return packet_module->resize(pkt, size);

	case MODE_PASSTHROUGH:
		error(L"operation not supported (pass-through mode)");
		return -1;

	default:
		assert(0);
		return -1;
	}
}

int packet_receive(struct packet_module_state *state, struct packet **pkt)
{
	int ret;
	assert(packet_module);

	ret = packet_module->receive(state, pkt);
	if (!ret && *pkt) {
		lua_object_init(&(*pkt)->lua_object);
		atomic_set(&(*pkt)->ref, 1);
		messagef(HAKA_LOG_DEBUG, L"packet", L"received packet id=%d, len=%u",
				packet_module->get_id(*pkt), packet_length(*pkt));
	}
	return ret;
}

void packet_drop(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	messagef(HAKA_LOG_DEBUG, L"packet", L"dropping packet id=%d, len=%u",
			packet_module->get_id(pkt), packet_length(pkt));
	packet_module->verdict(pkt, FILTER_DROP);
}

void packet_accept(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	messagef(HAKA_LOG_DEBUG, L"packet", L"accepting packet id=%d, len=%u",
			packet_module->get_id(pkt), packet_length(pkt));
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
		lua_object_release(pkt, &pkt->lua_object);
		packet_module->release_packet(pkt);
		return true;
	}
	else {
		return false;
	}
}

enum packet_status packet_state(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->packet_getstate(pkt);
}

void packet_set_mode(enum packet_mode mode)
{
	global_packet_mode = mode;
}

enum packet_mode packet_mode()
{
	return global_packet_mode;
}
