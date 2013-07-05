
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <haka/lua/object.h>
#include "../app.h"


extern int luaopen_app(lua_State *L);
extern int luaopen_module(lua_State *L);
extern int luaopen_packet(lua_State *L);
extern int luaopen_log(lua_State *L);
extern int luaopen_stream(lua_State *L);
extern int luaopen_luadebug(lua_State *L);

typedef struct {
	lua_CFunction open;
	const char   *name;
	const char   *rename;
} swig_module;

swig_module swig_builtins[] = {
	{ luaopen_app, "app", NULL },
	{ luaopen_module, "module", NULL },
	{ luaopen_packet, "packet", NULL },
	{ luaopen_log, "log", NULL },
	{ luaopen_stream, "stream", NULL },
	{ luaopen_luadebug, "luadebug", "debug" },
	{NULL, NULL}
};


struct lua_state *haka_init_state()
{
	swig_module *cur_module;
	struct lua_state *state = lua_state_init();
	if (!state) {
		return NULL;
	}

	lua_newtable(state->L);
	lua_setglobal(state->L, "haka");
	lua_getglobal(state->L, "haka");

	cur_module = swig_builtins;
	while (cur_module->open) {
		lua_getglobal(state->L, cur_module->name); /* save old value of global <name> */
		lua_setfield(state->L, LUA_REGISTRYINDEX, "swig_tmp");
		lua_pushcfunction(state->L, cur_module->open);
		lua_call(state->L, 0, 1);
		if (cur_module->rename) {
			lua_setfield(state->L, -2, cur_module->rename);
		}
		else {
			lua_setfield(state->L, -2, cur_module->name); /* haka.<name> = <module */
		}
		lua_getfield(state->L, LUA_REGISTRYINDEX, "swig_tmp"); /* restore old value of global <name> */
		lua_setglobal(state->L, cur_module->name);
		cur_module++;
	}
	lua_pushnil(state->L);
	lua_setfield(state->L, LUA_REGISTRYINDEX, "swig_tmp");

	return state;
}

void print_error(struct lua_State *L, const wchar_t *msg)
{
	if (msg)
		messagef(HAKA_LOG_ERROR, L"lua", L"%ls: %s", msg, lua_tostring(L, -1));
	else
		messagef(HAKA_LOG_ERROR, L"lua", L"%s", lua_tostring(L, -1));

	lua_pop(L, 1);
}

int run_file(struct lua_State *L, const char *filename, int argc, char *argv[])
{
	int i;
	if (luaL_loadfile(L, filename)) {
		print_error(L, NULL);
		return 1;
	}
	for (i = 1; i <= argc; i++) {
		lua_pushstring(L, argv[i-1]);
	}
	if (lua_pcall(L, argc, 0, 0)) {
		print_error(L, NULL);
		return 1;
	}

	return 0;
}

int do_file_as_function(struct lua_State *L, const char *filename)
{
	if (luaL_loadfile(L, filename)) {
		print_error(L, NULL);
		return -1;
	}

	if (lua_pcall(L, 0, 1, 0)) {
		print_error(L, NULL);
		return -1;
	}

	if (!lua_isfunction(L, -1)) {
		message(HAKA_LOG_ERROR, L"lua", L"script does not return a function");
		return -1;
	}

	return luaL_ref(L, LUA_REGISTRYINDEX);
}
