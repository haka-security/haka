
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include "../app.h"


extern int luaopen_app(lua_State *L);
extern int luaopen_module(lua_State *L);
extern int luaopen_packet(lua_State *L);
extern int luaopen_log(lua_State *L);

typedef struct {
	lua_CFunction open;
	const char * name;
} swig_module;

swig_module swig_builtins[] = {
	{ luaopen_app, "app" },
	{ luaopen_module, "module" },
	{ luaopen_packet, "packet" },
	{ luaopen_log, "log" },
	{NULL, NULL}
};


static int panic(lua_State *L)
{
	message(HAKA_LOG_FATAL, L"lua", L"lua panic");
	return 0;
}

/*
 * We need to override the print function to print using wprintf otherwise
 * nothing is visible in the output on Linux.
 */
static int lua_print(lua_State* L)
{
	int i;
	int nargs = lua_gettop(L);

	for (i=1; i<=nargs; i++) {
		if (i > 1)
			wprintf(L" ");

		if (lua_isstring(L, i)) {
			wprintf(L"%s", lua_tostring(L, i));
		}
		else {
			lua_getglobal(L, "tostring");
			lua_pushvalue(L, i);
			lua_call(L, 1, 1);
			if (lua_isstring(L, -1))
				wprintf(L"%s", lua_tostring(L, -1));
			else
				wprintf(L"(error)");
			lua_pop(L, 1);
		}
	}

	wprintf(L"\n");

	return 0;
}

lua_state *init_state()
{
	lua_State *L = luaL_newstate();
	if (!L) {
		return NULL;
	}

	lua_atpanic(L, panic);

	luaL_openlibs(L);

	lua_pushcfunction(L,lua_print);
	lua_setglobal(L,"print");

	lua_newtable(L);
	lua_setglobal(L,"haka");
	lua_getglobal(L,"haka");

	swig_module * cur_module = swig_builtins;
	while(cur_module->open) {
		lua_getglobal(L,cur_module->name); /* save old value of global <name> */
		lua_setfield(L,LUA_REGISTRYINDEX,"swig_tmp");
		lua_pushcfunction(L,cur_module->open);
		lua_call(L,0,1);
		lua_setfield(L,-2,cur_module->name); /* haka.<name> = <module */
		lua_getfield(L,LUA_REGISTRYINDEX,"swig_tmp"); /* restore old value of global <name> */
		lua_setglobal(L,cur_module->name);
		cur_module++;
	}
	lua_pushnil(L);
	lua_setfield(L,LUA_REGISTRYINDEX,"swig_tmp");

	return L;
}

void cleanup_state(lua_state *L)
{
	lua_close(L);
}

void print_error(lua_state *L, const wchar_t *msg)
{
	if (msg)
		messagef(HAKA_LOG_ERROR, L"lua", L"%ls: %s", msg, lua_tostring(L, -1));
	else
		messagef(HAKA_LOG_ERROR, L"lua", L"%s", lua_tostring(L, -1));
}

int run_file(lua_state *L, const char *filename, int argc, char *argv[])
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

int do_file_as_function(lua_state *L, const char *filename)
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
