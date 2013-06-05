
#include <assert.h>
#include <stdlib.h>

#include <haka/packet.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/packet_module.h>

extern void lua_invalidatepacket(struct packet *pkt);

static struct packet_module *packet_module = NULL;

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

uint8* packet_data_modifiable(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	return packet_module->make_modifiable(pkt);
}

int packet_resize(struct packet *pkt, size_t size)
{
	assert(packet_module);
	return packet_module->resize(pkt, size);
}

int packet_receive(struct packet_module_state *state, struct packet **pkt)
{
	int ret;
	assert(packet_module);

	ret = packet_module->receive(state, pkt);
	if (!ret && *pkt) {
		messagef(HAKA_LOG_DEBUG, L"packet", L"received packet id=%d", packet_module->get_id(*pkt));
	}
	return ret;
}

void packet_drop(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	lua_invalidatepacket(pkt);
	messagef(HAKA_LOG_DEBUG, L"packet", L"dropping packet id=%d", packet_module->get_id(pkt));
	packet_module->verdict(pkt, FILTER_DROP);
}

void packet_accept(struct packet *pkt)
{
	assert(packet_module);
	assert(pkt);
	lua_invalidatepacket(pkt);
	messagef(HAKA_LOG_DEBUG, L"packet", L"accepting packet id=%d", packet_module->get_id(pkt));
	packet_module->verdict(pkt, FILTER_ACCEPT);
}
