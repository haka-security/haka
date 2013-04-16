
#ifndef _APP_H
#define _APP_H

#include <haka/packet_module.h>
#include <haka/log_module.h>
#include "lua/state.h"


struct module;
typedef filter_result (*filter_callback)(lua_state *L, void *data, struct packet *pkt);

extern int set_packet_module(struct module *module);
extern struct packet_module *get_packet_module();
extern int set_log_module(struct module *module);
extern int has_log_module();

int set_filter(filter_callback filter, void *data);
int has_filter();
filter_result call_filter(lua_state *L, struct packet *pkt);

extern char directory[1024];

const char *get_app_directory();

#endif /* _APP_H */

