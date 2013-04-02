
#include <haka/packet_module.h>

#include <stdlib.h>
#include <assert.h>

#include "app.h"


static struct packet_module *packet_module = NULL;
static struct log_module *log_module = NULL;
static filter_callback filter_function;
static void *filter_data;


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

struct packet_module *get_packet_module()
{
	return packet_module;
}

int set_log_module(struct module *module)
{
	if (module) {
		if (module->type == MODULE_LOG) {
			log_module = (struct log_module *)module;
			return 0;
		}
		else
			return 1;
	}
	else {
		log_module = NULL;
		return 0;
	}
}

struct log_module *get_log_module()
{
	return log_module;
}

int set_filter(filter_callback filter, void *data)
{
    filter_function = filter;
    filter_data = data;
}

int has_filter()
{
    return filter_function != NULL;
}

filter_result call_filter(lua_State *L, struct packet *pkt)
{
    return filter_function(L, filter_data, pkt);
}

