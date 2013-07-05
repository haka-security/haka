
#include "utils.h"

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


static int execute_call(lua_State *L, int n) {
	int status;
	int h;

	h = lua_gettop(L) - n;
	lua_pushcfunction(L, lua_state_error_formater);
	lua_insert(L, h);

	status = lua_pcall(L, n, LUA_MULTRET, h);
	if (status) {
		printf(RED "%s\n" CLEAR, lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	lua_remove(L, h);

	return status;
}

int execute_print(lua_State *L)
{
	int i, g, h, status;
	LUA_STACK_MARK(L);

	g = lua_gettop(L);
	status = execute_call(L, 0);
	h = lua_gettop(L) - g + 1;

	lua_getglobal(L, "luadebug");
	lua_getfield(L, -1, "pprint");

	for (i = h ; i > 0 ; --i) {
		printf("  #%d\t", h-i+1);
		lua_pushvalue(L, -1);
		lua_pushvalue(L, -i-3);
		lua_pushstring(L, "    \t");
		lua_pushnil(L);
		lua_getfield(L, -6, "hide_underscore");
		if (lua_pcall(L, 4, 0, 0)) {
			printf("%s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}

	lua_pop(L, 2);
	lua_settop(L, g);

	LUA_STACK_CHECK(L, 0);
	return status;
}

int capture_env(struct lua_State *L, int frame)
{
	int i;
	const char *local;
	lua_Debug ar;
	LUA_STACK_MARK(L);

	if (!lua_getstack(L, frame, &ar)) {
		return -1;
	}

	lua_getinfo(L, "f", &ar);

	lua_newtable(L);

	for (i=1; (local = lua_getlocal(L, &ar, i)); ++i) {
		lua_setfield(L, -2, local);
	}

	for (i=1; (local = lua_getupvalue(L, -2, i)); ++i) {
		lua_setfield(L, -2, local);
	}

	lua_newtable(L);
	lua_getfenv(L, -3);
	lua_setfield(L, -2, "__index");
	lua_getfenv(L, -3);
	lua_setfield(L, -2, "__newindex");
	lua_setmetatable(L, -2);

	lua_remove(L, -2);

	LUA_STACK_CHECK(L, 1);

	return lua_gettop(L);
}
