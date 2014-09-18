/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <haka/error.h>
#include <haka/types.h>
#include <haka/lua/marshal.h>
#include <haka/lua/luautils.h>
#include <haka/lua/state.h>

int mar_encode(lua_State* L);
int mar_decode(lua_State* L);

char *lua_marshal(struct lua_State *L, int index, size_t *len)
{
	char *ret = NULL;
	int h;

	LUA_STACK_MARK(L);

	lua_pushcfunction(L, lua_state_error_formater);
	h = lua_gettop(L);

	lua_pushcfunction(L, mar_encode);
	lua_pushvalue(L, index);
	if (lua_pcall(L, 1, 1, h)) {
		error("%s", lua_tostring(L, -1));
	}
	else {
		if (lua_isstring(L, -1)) {
			const char *ptr = lua_tolstring(L, h+1, len);
			ret = malloc(*len);
			if (!ret) {
				error("memory error");
			}
			else {
				memcpy(ret, ptr, *len);
			}
		}
		else {
			error("marshaling error");
		}
	}

	lua_pop(L, 2);
	LUA_STACK_CHECK(L, 0);
	return ret;
}

bool lua_unmarshal(struct lua_State *L, const char *data, size_t len)
{
	int h;

	LUA_STACK_MARK(L);

	lua_pushcfunction(L, lua_state_error_formater);
	h = lua_gettop(L);

	lua_pushcfunction(L, mar_decode);
	lua_pushlstring(L, data, len);
	if (lua_pcall(L, 1, 1, h)) {
		error("%s", lua_tostring(L, -1));
	}

	lua_remove(L, h);
	LUA_STACK_CHECK(L, 1);
	return true;
}
