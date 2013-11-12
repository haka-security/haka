
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <haka/lua/object.h>
#include <luadebug/lua.h>
#include "../app.h"


extern int luaopen_haka(lua_State *L);
extern int luaopen_log(lua_State *L);
extern int luaopen_alert(lua_State *L);
extern int luaopen_packet(lua_State *L);

typedef struct {
	lua_CFunction open;
	const char   *name;
} swig_module;

swig_module swig_builtins[] = {
	{ luaopen_packet, "packet" },
	{ luaopen_log, "log" },
	{ luaopen_alert, "alert" },
	{ luaopen_luadebug, "debug" },
	{NULL, NULL}
};


struct lua_state *haka_init_state()
{
	struct lua_state *state = lua_state_init();
	if (!state) {
		return NULL;
	}

	luaopen_haka(state->L);

	{
		swig_module *cur_module = swig_builtins;
		while (cur_module->open) {
			lua_pushcfunction(state->L, cur_module->open);
			lua_call(state->L, 0, 1);
			lua_setfield(state->L, -2, cur_module->name);
			cur_module++;
		}
	}

	return state;
}

int run_file(struct lua_State *L, const char *filename, int argc, char *argv[])
{
	int i;
	if (luaL_loadfile(L, filename)) {
		lua_state_print_error(L, NULL);
		return 1;
	}
	for (i = 1; i <= argc; i++) {
		lua_pushstring(L, argv[i-1]);
	}
	if (lua_pcall(L, argc, 0, 0)) {
		lua_state_print_error(L, NULL);
		return 1;
	}

	return 0;
}

int do_file_as_function(struct lua_State *L, const char *filename)
{
	if (luaL_loadfile(L, filename)) {
		lua_state_print_error(L, NULL);
		return -1;
	}

	if (lua_pcall(L, 0, 1, 0)) {
		lua_state_print_error(L, NULL);
		return -1;
	}

	if (!lua_isfunction(L, -1)) {
		message(HAKA_LOG_ERROR, L"lua", L"script does not return a function");
		return -1;
	}

	return luaL_ref(L, LUA_REGISTRYINDEX);
}
