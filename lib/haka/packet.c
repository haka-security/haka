
#include <assert.h>

#include <haka/packet.h>
#include <haka/packet_module.h>


static struct packet_module *packet_module = NULL;

int set_packet_module(struct module *module)
{
	if (module) {
		if (module->type == MODULE_PACKET) {
			packet_module = (struct packet_module *)module;
			return 0;
        }
		else
			return 1;
	}
	else {
		packet_module = NULL;
		return 0;
	}
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

const char *packet_data(struct packet *pkt)
{
    assert(packet_module);
    assert(pkt);
    return packet_module->get_data(pkt);
}

