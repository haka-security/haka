
#ifndef _APP_H
#define _APP_H

#include <haka/packet_module.h>
#include <lua.h>


struct module;
typedef filter_result (*filter_callback)(lua_State *L, void *data, struct packet *pkt);

int set_packet_module(struct module *module);
struct packet_module *get_packet_module();
int set_log_module(struct module *module);
struct log_module *get_log_module();

int set_filter(filter_callback filter, void *data);
int has_filter();
filter_result call_filter(lua_State *L, struct packet *pkt);

#endif /* _APP_H */

