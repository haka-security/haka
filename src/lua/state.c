
#include <lua.h>
#include <stdlib.h>
#include <stdio.h>


extern int openlua_app(lua_State *L);
extern int openlua_module(lua_State *L);
extern int openlua_packet(lua_State *L);


static int panic(lua_State *L)
{
	return 0;
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

	return L;
}

void cleanup_state(lua_State *L)
{
	lua_close(L);
}

void print_error(const char *msg, lua_State *L)
{
	fprintf(stderr, "%s: %s\n", msg, lua_tostring(L, -1));
}

