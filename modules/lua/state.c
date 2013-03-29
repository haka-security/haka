
#include <lua.h>
#include <stdlib.h>


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
	lua_State *state = lua_newstate(alloc, NULL);
	if (!state) {
		return NULL;
	}

	lua_atpanic(state, panic);

	return state;
}

void cleanup_state(lua_State *state)
{
	lua_close(state);
}

