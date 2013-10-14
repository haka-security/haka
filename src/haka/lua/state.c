
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
extern int luaopen_packet(lua_State *L);
extern int luaopen_state_machine(lua_State *L);

typedef struct {
	lua_CFunction open;
	const char   *name;
} swig_module;

swig_module swig_builtins1[] = {
	{ luaopen_log, "log" },
	{ luaopen_luadebug, "debug" },
	{NULL, NULL}
};

swig_module swig_builtins2[] = {
	{ luaopen_packet, "packet" },
	{ luaopen_state_machine, "state_machine" },
	{NULL, NULL}
};

static void load_modules(struct lua_State *L, swig_module *modules)
{
	swig_module *cur_module = modules;
	while (cur_module->open) {
		lua_pushcfunction(L, cur_module->open);
		lua_call(L, 0, 1);
		lua_setfield(L, -2, cur_module->name);
		cur_module++;
	}
}

struct lua_state *haka_init_state()
{
	struct lua_state *state = lua_state_init();
	if (!state) {
		return NULL;
	}

	luaopen_haka(state->L);

	load_modules(state->L, swig_builtins1);

	lua_getfield(state->L, -1, "initialize");
	if (lua_pcall(state->L, 0, 0, 0)) {
		lua_state_print_error(state->L, NULL);
		lua_state_close(state);
		return NULL;
	}

	load_modules(state->L, swig_builtins2);

	lua_pop(state->L, 1);

	return state;
}

int run_file(struct lua_State *L, const char *filename, int argc, char *argv[])
{
	int i, h;

	lua_pushcfunction(L, lua_state_error_formater);
	h = lua_gettop(L);

	if (luaL_loadfile(L, filename)) {
		lua_state_print_error(L, NULL);
		lua_pop(L, 1);
		return 1;
	}
	for (i = 1; i <= argc; i++) {
		lua_pushstring(L, argv[i-1]);
	}
	if (lua_pcall(L, argc, 0, h)) {
		lua_state_print_error(L, NULL);
		lua_pop(L, 1);
		return 1;
	}

	lua_pop(L, 1);
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
