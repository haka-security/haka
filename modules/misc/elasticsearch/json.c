/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "json.h"

#include <jansson.h>
#include <lua.h>

#include <haka/error.h>
#include <haka/lua/luautils.h>


static json_t *lua_element_to_json(struct lua_State *L, int index)
{
	json_t *ret = NULL;
	const int val_type = lua_type(L, index);
	const int h = lua_gettop(L);

	lua_pushvalue(L, index);

	switch (val_type) {
	case LUA_TBOOLEAN: {
		const int int_val = lua_toboolean(L, -1);
		ret = int_val ? json_true() : json_false();
		if (!ret) error("json boolean conversion error");
		break;
	}
	case LUA_TSTRING:
	case LUA_TUSERDATA: {
		const char *str_val = lua_converttostring(L, -1, NULL);
		if (!str_val) {
			error("cannot convert value to string");
			break;
		}

		ret = json_string(str_val);
		if (!ret) error("json string conversion error");
		break;
	}
	case LUA_TNUMBER: {
		lua_Number num_val = lua_tonumber(L, -1);
		ret = json_real(num_val);
		if (!ret) error("json number conversion error");
		break;
	}
	case LUA_TTABLE: {
		ret = json_object();
		if (!ret) {
			error("json table conversion error");
			break;
		}

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			size_t len;
			const char *str_val = lua_tolstring(L, -2, &len);
			json_t *val = lua_element_to_json(L, -1);
			if (!val) {
				json_decref(ret);
				ret = NULL;
				break;
			}

			if (json_object_set(ret, str_val, val) != 0) {
				error("json table conversion error");
				json_decref(val);
				json_decref(ret);
				ret = NULL;
				break;
			}

			lua_pop(L, 1);
		}

		break;
	}
	case LUA_TFUNCTION: {
		error("function cannot be converted to json");
		break;
	}
	case LUA_TNIL: {
		ret = json_null();
		if (!ret) error("json nil conversion error");
		break;
	}
	default:
		error("invalid value type (%s)", lua_typename(L, val_type));
	}

	lua_settop(L, h);
	return ret;
}

json_t *lua2json(struct lua_State *L, int index)
{
	return lua_element_to_json(L, index);
}
