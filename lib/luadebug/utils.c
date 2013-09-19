
#include "utils.h"

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>
#include <luadebug/user.h>

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

static int lua_user_print(lua_State *L)
{
	int i;
	int nargs = lua_gettop(L);
	struct luadebug_user *user;

	assert(lua_islightuserdata(L, 1));
	user = (struct luadebug_user *)lua_topointer(L, 1);
	assert(user);

	for (i=2; i<=nargs; i++) {
		if (i > 2)
			user->print(user, " ");

		user->print(user, "%s", lua_converttostring(L, i, NULL));
		lua_pop(L, 1);
	}

	user->print(user, "\n");

	return 0;
}

void pprint(lua_State *L, struct luadebug_user *user, int index, bool full, const char *hide)
{
	int h;
	LUA_STACK_MARK(L);

	lua_getglobal(L, "haka");
	lua_getfield(L, -1, "debug");
	h = lua_gettop(L);

	if (user) {
		lua_getfield(L, h, "__printwrapper");
		lua_pushcfunction(L, lua_user_print);
		lua_pushlightuserdata(L, user);
		if (lua_pcall(L, 2, 1, 0)) {
			user->print(user, CLEAR RED "error: %s\n" CLEAR, lua_tostring(L, -1));
			lua_settop(L, h-2);
			LUA_STACK_CHECK(L, 0);
			return;
		}
	}

	lua_getfield(L, h, "pprint");

	/* obj */
	if (index < 0) {
		lua_pushvalue(L, h-1+index);
	}
	else {
		lua_pushvalue(L, index);
	}

	/* indent */
	lua_pushstring(L, "    \t");

	/* depth */
	if (!full) {
		lua_pushnumber(L, 1);
	}
	else {
		lua_pushnil(L);
	}

	/* hide */
	if (hide) {
		lua_getfield(L, h, "hide_underscore");
	}
	else {
		lua_pushnil(L);
	}

	/* out */
	if (user) {
		lua_pushvalue(L, h+1);
	}
	else {
		lua_pushnil(L);
	}

	if (lua_pcall(L, 5, 0, 0)) {
		user->print(user, CLEAR RED "error: %s\n" CLEAR, lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	lua_settop(L, h-2);

	LUA_STACK_CHECK(L, 0);
}

int execute_print(lua_State *L, struct luadebug_user *user)
{
	int i, g, h, status;
	LUA_STACK_MARK(L);

	g = lua_gettop(L);
	status = execute_call(L, 0);
	h = lua_gettop(L) - g + 1;

	for (i = h; i > 0; --i) {
		user->print(user, "  #%d\t", h-i+1);
		pprint(L, user, -i, true, "hide_underscore");
	}

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
