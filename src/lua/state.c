
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


lua_State *init_state()
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

void cleanup_state(lua_State *L)
{
	lua_close(L);
}

void print_error(const wchar_t *msg, lua_State *L)
{
    messagef(LOG_FATAL, L"lua", L"%ls: %s", msg, lua_tostring(L, -1));
}

