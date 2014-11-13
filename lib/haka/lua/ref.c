/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/lua/luautils.h>
#include <haka/lua/ref.h>
#include <haka/lua/state.h>
#include <haka/lua/object.h>
#include <haka/error.h>

#include <assert.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua/lua/ref.h"

#define REF_TABLE         "__ref"
#define WEAKREF_TABLE     "__weak_ref"
#define WEAKREF_ID_TABLE  "__weak_ref_id"


void lua_ref_init_state(lua_State *L)
{
	LUA_STACK_MARK(L);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, REF_TABLE);

	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, WEAKREF_TABLE);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, WEAKREF_ID_TABLE);

	LUA_STACK_CHECK(L, 0);

	lua_load_ref(L);
	lua_getfield(L, LUA_REGISTRYINDEX, REF_TABLE);
	lua_getfield(L, LUA_REGISTRYINDEX, WEAKREF_TABLE);
	lua_getfield(L, LUA_REGISTRYINDEX, WEAKREF_ID_TABLE);
	lua_call(L, 3, 0);
}

const struct lua_ref lua_ref_init = LUA_REF_INIT;

bool lua_ref_isvalid(struct lua_ref *ref)
{
	return (ref->state && ref->ref != LUA_NOREF);
}

void lua_ref_get(struct lua_State *state, struct lua_ref *ref, int index, bool weak)
{
	LUA_STACK_MARK(state);

	if (index < 0) {
		index = lua_gettop(state)+index+1;
	}

	lua_ref_clear(state, ref);

	if (!lua_isnil(state, index)) {
		ref->state = lua_state_get(state);
		ref->weak = weak;

		if (index < 0) {
			index = lua_gettop(state)+index+1;
		}

		if (weak) {
			lua_getfield(state, LUA_REGISTRYINDEX, WEAKREF_ID_TABLE);
			lua_pushboolean(state, true);
			ref->ref = luaL_ref(state, -2);
			lua_pop(state, 1);

			lua_getfield(state, LUA_REGISTRYINDEX, WEAKREF_TABLE);
			lua_pushvalue(state, index);
			lua_rawseti(state, -2, ref->ref);
		}
		else {
			lua_getfield(state, LUA_REGISTRYINDEX, REF_TABLE);
			lua_pushvalue(state, index);
			ref->ref = luaL_ref(state, -2);
		}

		lua_pop(state, 1);
	}

	LUA_STACK_CHECK(state, 0);
}

static int lua_ref_delay_clear(lua_State *L)
{
	assert(lua_islightuserdata(L, 1));
	struct lua_ref *ref = lua_touserdata(L, 1);
	lua_ref_clear(L, ref);
	return 0;
}

bool lua_ref_clear(struct lua_State *state, struct lua_ref *ref)
{
	if (lua_ref_isvalid(ref)) {
		if (lua_state_isvalid(ref->state)) {
			if (state && lua_state_get(state) == ref->state) {
				LUA_STACK_MARK(ref->state->L);

				if (ref->weak) {
					lua_getfield(ref->state->L, LUA_REGISTRYINDEX, WEAKREF_TABLE);
					lua_pushnil(ref->state->L);
					lua_rawseti(ref->state->L, -2, ref->ref);
					lua_pop(ref->state->L, 1);

					lua_getfield(ref->state->L, LUA_REGISTRYINDEX, WEAKREF_ID_TABLE);
				}
				else {
					lua_getfield(ref->state->L, LUA_REGISTRYINDEX, REF_TABLE);
				}

				luaL_unref(ref->state->L, -1, ref->ref);
				lua_pop(ref->state->L, 1);

				LUA_STACK_CHECK(ref->state->L, 0);
			}
			else {
				struct lua_ref *ref_copy = malloc(sizeof(struct lua_ref));
				if (!ref_copy) {
					error("memory error");
					return false;
				}

				*ref_copy = *ref;

				if (!lua_state_interrupt(ref->state, lua_ref_delay_clear, ref_copy, free)) {
					free(ref_copy);
					return false;
				}

				*ref = lua_ref_init;
			}
		}

		ref->ref = LUA_NOREF;
		ref->state = NULL;
		return true;
	}
	else {
		return false;
	}
}

void lua_ref_push(struct lua_State *state, struct lua_ref *ref)
{
	LUA_STACK_MARK(state);

	if (lua_ref_isvalid(ref)) {
		assert(lua_state_get(state) == ref->state);

		lua_getfield(state, LUA_REGISTRYINDEX, ref->weak ? WEAKREF_TABLE : REF_TABLE);
		assert(lua_istable(state, -1));

		lua_rawgeti(state, -1, ref->ref);
		lua_remove(state, -2);
	}
	else {
		lua_pushnil(state);
	}

	LUA_STACK_CHECK(state, 1);
}

struct lua_ref *lua_object_get_ref(void *_obj)
{
	struct lua_object *obj = (struct lua_object *)_obj;
	return &obj->ref;
}
