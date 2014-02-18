/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module object

%include "haka/lua/swig.si"

%{
#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/lua/object.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>

#include <assert.h>
#include <stdio.h>
#include <wchar.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


#define OBJECT_TABLE "__haka_objects"


void lua_object_initialize(lua_State *L)
{
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
}

bool lua_object_ownedbylua(struct lua_object *obj)
{
	assert(obj);

	if (!obj->state) {
		return false;
	}

	{
		LUA_STACK_MARK(obj->state->L);

		lua_getfield(obj->state->L, LUA_REGISTRYINDEX, OBJECT_TABLE);
		lua_pushlightuserdata(obj->state->L, obj);
		lua_gettable(obj->state->L, -2);
		if (lua_isnil(obj->state->L, -1)) {
			lua_pop(obj->state->L, 2);
			LUA_STACK_CHECK(obj->state->L, 0);
			return false;
		}

		swig_lua_userdata *usr = (swig_lua_userdata*)lua_touserdata(obj->state->L, -1);
		lua_pop(obj->state->L, 2);
		LUA_STACK_CHECK(obj->state->L, 0);

		if (usr) {
			return usr->own == 1;
		}

		return false;
	}
}

const struct lua_object lua_object_init = LUA_OBJECT_INIT;

void lua_object_release(void *ptr, struct lua_object *obj)
{
	assert(obj);

	if (obj->state) {
		if (lua_state_isvalid(obj->state)) {
			lua_State *L = obj->state->L;
			obj->state = NULL;

			LUA_STACK_MARK(L);

			lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
			assert(lua_istable(L, -1));

			lua_pushlightuserdata(L, obj);
			lua_gettable(L, -2);
			lua_pushnil(L);

			assert(!lua_isnil(L, -2));
			assert(lua_istable(L, -2));

			while (lua_next(L, -2)) {
				swig_lua_userdata *usr;
				swig_lua_class *clss;

				if (!lua_isnil(L, -1)) {
					assert(lua_isuserdata(L, -1));
					usr = (swig_lua_userdata*)lua_touserdata(L, -1);
					assert(usr);
					assert(usr->ptr);

					if (usr->ptr != ptr && usr->own) {
						clss = (swig_lua_class*)usr->type->clientdata;
						if (clss && clss->destructor)
						{
							clss->destructor(usr->ptr);
						}
					}

					usr->ptr = NULL;
				}

				lua_pop(L, 1);
			}

			lua_pop(L, 1);
			lua_pushlightuserdata(L, obj);
			lua_pushnil(L);
			lua_settable(L, -3);
			lua_pop(L, 1);

			LUA_STACK_CHECK(L, 0);
		}
		else {
			obj->state = NULL;
		}
	}
}

bool lua_object_get(lua_State *L, struct lua_object *obj, swig_type_info *type_info)
{
	assert(obj);

	if (obj->state && obj->state->L != L) {
		lua_pushstring(L, "invalid lua state (an object is being used by multiple lua state)");
		return false;
	}

	LUA_STACK_MARK(L);

	lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
		lua_pushnil(L);
		LUA_STACK_CHECK(L, 1);
		return true;
	}
	else {
		lua_pushstring(L, type_info->name);
		lua_gettable(L, -2);
		lua_remove(L, -2);
		lua_remove(L, -2);
		LUA_STACK_CHECK(L, 1);
		return true;
	}
}

void lua_object_register(lua_State *L, struct lua_object *obj, swig_type_info *type_info, int index)
{
	if (!obj->state) {
		obj->state = lua_state_get(L);
	}
	else {
		/* A given lua object can only belong to 1 lua state */
		assert(obj->state->L == L);
	}

	LUA_STACK_MARK(L);

	lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pushlightuserdata(L, obj);

		lua_newtable(L);

		/* Make the values of this table weak refs */
		lua_newtable(L);
		lua_pushstring(L, "v");
		lua_setfield(L, -2, "__mode");
		lua_setmetatable(L, -2);

		lua_settable(L, -3);

		lua_pushlightuserdata(L, obj);
		lua_gettable(L, -2);
	}

	lua_pushstring(L, type_info->name);
	lua_pushvalue(L, index-3);
	lua_settable(L, -3);
	lua_pop(L, 2);

	LUA_STACK_CHECK(L, 0);
}

bool lua_object_push(lua_State *L, void *ptr, struct lua_object *obj, swig_type_info *type_info, int owner)
{
	if (ptr) {
		if (!lua_object_get(L, obj, type_info)) {
			return false;
		}

		if (!lua_isnil(L, -1)) {
			swig_lua_userdata *usr;
			assert(lua_isuserdata(L, -1));
			usr = (swig_lua_userdata *)lua_touserdata(L, -1);
			assert(SWIG_TypeCheckStruct(usr->type, type_info));

			if (owner) {
				usr->own = 1;
			}
		}
		else {
			if (ptr) {
				lua_pop(L, 1);
				SWIG_NewPointerObj(L, ptr, type_info, owner);

				lua_object_register(L, obj, type_info, -1);
			}
		}

		return true;
	}
	else {
		lua_pushnil(L);
		return true;
	}
}

%}
