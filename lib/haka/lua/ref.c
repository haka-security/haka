/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/types.h>
#include <haka/lua/ref.h>
#include <haka/lua/state.h>

#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


void lua_ref_init(struct lua_ref *ref)
{
	ref->state = NULL;
	ref->ref = LUA_NOREF;
}

bool lua_ref_isvalid(struct lua_ref *ref)
{
	return (ref->state && ref->ref != LUA_NOREF);
}

void lua_ref_get(struct lua_State *state, struct lua_ref *ref)
{
	lua_ref_clear(ref);

	if (!lua_isnil(state, -1)) {
		ref->state = lua_state_get(state);
		ref->ref = luaL_ref(state, LUA_REGISTRYINDEX);
	}
}

bool lua_ref_clear(struct lua_ref *ref)
{
	if (lua_ref_isvalid(ref)) {
		if (lua_state_isvalid(ref->state)) {
			luaL_unref(ref->state->L, LUA_REGISTRYINDEX, ref->ref);
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
		assert(state == ref->state->L);
		lua_rawgeti(state, LUA_REGISTRYINDEX, ref->ref);
	}
	else {
		lua_pushnil(state);
	}
}
