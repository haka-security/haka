/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/lua/ref.h>
#include <haka/lua/state.h>

#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define REF_TABLE      "__ref"
#define WEAKREF_TABLE  "__weak_ref"


void lua_ref_init_state(lua_State *L)
{
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, REF_TABLE);

	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, WEAKREF_TABLE);
}

void lua_ref_init(struct lua_ref *ref)
{
	ref->state = NULL;
	ref->ref = LUA_NOREF;
	ref->weak = false;
}

bool lua_ref_isvalid(struct lua_ref *ref)
{
	return (ref->state && ref->ref != LUA_NOREF);
}

void lua_ref_get(struct lua_State *state, struct lua_ref *ref, int index, bool weak)
{
	lua_ref_clear(ref);

	if (!lua_isnil(state, index)) {
		ref->state = lua_state_get(state);
		ref->weak = weak;

		lua_getfield(state, LUA_REGISTRYINDEX, weak ? WEAKREF_TABLE : REF_TABLE);
		assert(lua_istable(state, -1));

		lua_pushvalue(state, index);
		ref->ref = luaL_ref(state, -2);
		lua_pop(state, 1);
	}
}

bool lua_ref_clear(struct lua_ref *ref)
{
	if (lua_ref_isvalid(ref)) {
		if (lua_state_isvalid(ref->state) && !ref->weak) {
			lua_getfield(ref->state->L, LUA_REGISTRYINDEX, ref->weak ? WEAKREF_TABLE : REF_TABLE);
			assert(lua_istable(ref->state->L, -1));

			luaL_unref(ref->state->L, -1, ref->ref);
			lua_pop(ref->state->L, 1);
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
}
