/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/lua/object.h>
#include <haka/config.h>
#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/error.h>
#include <haka/lua/object.h>
#include <haka/lua/state.h>
#include <haka/lua/luautils.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


#define OBJECT_TABLE "__c_obj_ref"

typedef struct {
  void   *type;
  int     own;  /* 1 if owned & must be destroyed */
  void        *ptr;
} swig_lua_userdata;


void lua_object_initialize(lua_State *L)
{
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
}

const struct lua_object lua_object_init = LUA_OBJECT_INIT;

#ifdef HAKA_FFI
static int lua_object_delay_release(lua_State *L)
{
	LUA_STACK_MARK(L);

	assert(lua_islightuserdata(L, 1));
	struct lua_object *obj = lua_touserdata(L, 1);

	if (obj->keep) {
		lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
		lua_pushnil(L);
		lua_rawseti(L, -2, obj->ref.ref);
		lua_pop(L, 1);
	}

	lua_ref_clear(L, &obj->ref);

	LUA_STACK_CHECK(L, 0);
	return 0;
}
#endif

void lua_object_release(struct lua_object *obj)
{
	assert(obj);

	if (obj->ref.state && lua_state_isvalid(obj->ref.state)) {
#ifdef HAKA_FFI
		struct lua_object *obj_copy = malloc(sizeof(struct lua_object));
		if (!obj_copy) {
			error("memory error");
			return;
		}

		*obj_copy = *obj;

		if (!lua_state_interrupt(obj->ref.state, lua_object_delay_release, obj_copy, free)) {
			free(obj_copy);
			return;
		}
#else
		lua_State *L = obj->ref.state->L;

		LUA_STACK_MARK(L);

		if (obj->keep) {
			lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
			lua_pushnil(L);
			lua_rawseti(L, -2, obj->ref.ref);
			lua_pop(L, 1);
		}

		lua_ref_push(L, &obj->ref);
		swig_lua_userdata *usr = (swig_lua_userdata*)lua_touserdata(L, -1);
		if (usr) usr->ptr = NULL;
		lua_pop(L, 1);

		lua_ref_clear(L, &obj->ref);

		LUA_STACK_CHECK(L, 0);
#endif

		*obj = lua_object_init;
	}
}

void lua_object_register(lua_State *L, struct lua_object *obj, int index, bool lua_own)
{
	LUA_STACK_MARK(L);

	if (index < 0) {
		index = lua_gettop(L)+index+1;
	}

	if (!lua_ref_isvalid(&obj->ref)) {
		lua_ref_get(L, &obj->ref, index, true);
	}

	if (!lua_own) {
		lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);

#ifdef HAKA_DEBUG
		assert(!obj->keep);
		lua_rawgeti(L, -1, obj->ref.ref);
		assert(lua_isnil(L, -1));
		lua_pop(L, 1);
#endif

		lua_pushvalue(L, index);
		lua_rawseti(L, -2, obj->ref.ref);
		lua_pop(L, 1);
		obj->keep = true;
	}

	assert(obj->ref.state == lua_state_get(L));

	LUA_STACK_CHECK(L, 0);
}

bool lua_object_push(lua_State *L, struct lua_object *obj, bool lua_own)
{
	LUA_STACK_MARK(L);

	if (!obj) {
		lua_pushnil(L);
		return true;
	}

	if (lua_ref_isvalid(&obj->ref)) {
		lua_ref_push(L, &obj->ref);
		assert(!lua_isnil(L, -1));

		if (lua_own) {
			lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);

#ifdef HAKA_DEBUG
			assert(obj->keep);
			lua_rawgeti(L, -1, obj->ref.ref);
			assert(!lua_isnil(L, -1));
			lua_pop(L, 1);
#endif

			lua_pushnil(L);
			lua_rawseti(L, -2, obj->ref.ref);
			lua_pop(L, 1);
			obj->keep = false;
		}

		LUA_STACK_CHECK(L, 1);
		return true;
	}
	else {
		return false;
	}
}
