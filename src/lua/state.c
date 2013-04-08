
#include <lua.h>
#include <stdlib.h>
#include <stdio.h>
#include "../app.h"


extern int openlua_app(lua_State *L);
extern int openlua_module(lua_State *L);
extern int openlua_packet(lua_State *L);
extern int openlua_log(lua_State *L);


static int panic(lua_State *L)
{
	message(LOG_FATAL, L"lua", L"lua panic");
}

static void *alloc(void *up, void *ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		free(ptr);
		return NULL;
	}
	else {
		return realloc(ptr, nsize);
	}
}


lua_state *init_state()
{
	lua_State *L = lua_newstate(alloc, NULL);
	if (!L) {
		return NULL;
	}

	lua_atpanic(L, panic);

	luaL_openlibs(L);
	luaopen_app(L);
	luaopen_module(L);
	luaopen_packet(L);
	luaopen_log(L);

	return L;
}

void cleanup_state(lua_state *L)
{
	lua_close(L);
}

void print_error(lua_state *L, const wchar_t *msg)
{
	if (msg)
		messagef(LOG_ERROR, L"lua", L"%ls: %s", msg, lua_tostring(L, -1));
	else
		messagef(LOG_ERROR, L"lua", L"%s", lua_tostring(L, -1));
}

int run_file(lua_state *L, const char *filename, int argc, char *argv[])
{
	int i;

	/* Create arg table containing command line arguments */
	lua_createtable(L, argc, 0);
	for (i = 1; i <= argc; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, argv[i-1]);
		lua_rawset(L, -3);      /* Stores the pair in the table */
	}
	lua_setglobal(L, "arg");

	if (luaL_loadfile(L, filename)) {
		print_error(L, NULL);
		return 1;
	}

	if (lua_pcall(L, 0, 0, 0)) {
		print_error(L, NULL);
		return 1;
	}

	return 0;
}
