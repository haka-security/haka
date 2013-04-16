%module app
%{
#include <stdint.h>
#include <wchar.h>

#include "app.h"
#include "state.h"
#include <haka/packet_module.h>

static int install_impl(const char *type, struct module *module)
{
	if (strcmp(type, "packet") == 0) {
		return set_packet_module(module);
	}
	else if (strcmp(type, "log") == 0) {
		return set_log_module(module);
	}
	else {
		return 2;
	}
}

#define install(type, module) \
	do { \
		const int ret = install_impl(type, module); \
		switch (ret) { \
		case 1: lua_pushfstring(L, "(arg %d) must implement module type '%s'", 2, type); SWIG_fail; \
		case 2: lua_pushfstring(L, "unknown module type '%s'", type); SWIG_fail; \
		} \
	} while(0)

extern void lua_pushppacket(lua_State *L, struct packet *pkt);

static filter_result lua_filter_wrapper(lua_state *L, void *data, struct packet *pkt)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, (intptr_t)data);
	lua_pushppacket(L, pkt);
	if (lua_pcall(L, 1, 1, 0)) {
		print_error(L, L"filter function");
		return FILTER_DROP;
	}

	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return FILTER_DROP;
	}

	const int ret = lua_tonumber(L, -1);
	lua_remove(L, -1);

	return (filter_result)ret;
}

int install_filter_native(lua_State *L)
{
	int SWIG_arg = 0;

	SWIG_check_num_args("install_filter",1,1)
	const intptr_t callback_function = luaL_ref(L, LUA_REGISTRYINDEX);
	set_filter(lua_filter_wrapper, (void*)callback_function);
	return SWIG_arg;

fail:
	lua_error(L);
	return SWIG_arg;
}

%}

void install(const char *type, struct module *module);
%native(install_filter) void install_filter_native(lua_State *L);
void exit(int);

%rename(directory) get_app_directory;
const char *get_app_directory();

%luacode {
	package.cpath = package.cpath .. ";" .. app.directory() .. "/modules/?.ho"
	package.path = package.path .. ";" .. app.directory() .. "/modules/?.lua"
}
