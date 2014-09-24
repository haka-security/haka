/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>

#include <haka/lua/luautils.h>
#include <haka/error.h>


/*
 * Redefine the luaL_tolstring (taken from Lua 5.2)
 */
const char *lua_converttostring(struct lua_State *L, int idx, size_t *len)
{
	if (!luaL_callmeta(L, idx, "__tostring")) {  /* no metafield? */
		switch (lua_type(L, idx)) {
			case LUA_TNUMBER:
			case LUA_TSTRING:
				lua_pushvalue(L, idx);
				break;
			case LUA_TBOOLEAN:
				lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
				break;
			case LUA_TNIL:
				lua_pushliteral(L, "nil");
				break;
			default:
				lua_pushfstring(L, "%s: %p", luaL_typename(L, idx),
						lua_topointer(L, idx));
				break;
		}
	}
	return lua_tolstring(L, -1, len);
}

bool lua_pushwstring(struct lua_State *L, const wchar_t *str)
{
	int size;
	char *strmb;

	size = wcstombs(NULL, str, 0);
	if (size == (size_t)-1) {
		error("unknown error");
		return false;
	}

	strmb = malloc(size+1);
	if (!strmb) {
		error("memory error");
		return false;
	}

	wcstombs(strmb, str, size+1);
	lua_pushstring(L, strmb);
	free(strmb);

	return true;
}
