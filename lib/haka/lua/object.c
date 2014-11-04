/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/lua/object.h>
#include <haka/config.h>
#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/lua/object.h>
#include <haka/lua/state.h>
#include <haka/lua/luautils.h>

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

const struct lua_object lua_object_init = LUA_OBJECT_INIT;

void lua_object_release(void *ptr, struct lua_object *obj)
{
	assert(obj);

	if (obj->owner) {
		assert(obj->ref.state);
		*obj->owner = NULL;
		obj->owner = NULL;
	}

	if (obj->ref.state && lua_state_isvalid(obj->ref.state)) {
		lua_ref_clear(NULL, &obj->ref);
	}
}

void lua_object_register(lua_State *L, struct lua_object *obj, void **owner, int index, bool disown)
{
	struct lua_state *mainthread = lua_state_get(L);

	if (!lua_ref_isvalid(&obj->ref)) {
		lua_ref_get(mainthread->L, &obj->ref, index, !disown);
	}
	else {
		if (disown) {
			struct lua_ref tmp = lua_ref_init;
			assert(obj->ref.weak);
			lua_ref_get(L, &tmp, index, false);
			lua_ref_clear(L, &obj->ref);
			obj->ref = tmp;
		}
	}

	if (owner && !obj->owner) {
		obj->owner = owner;
	}

	assert(obj->owner == owner);
	assert(obj->ref.state == mainthread);
}

bool lua_object_push(lua_State *L, void *ptr, struct lua_object *obj, bool owner)
{
	if (ptr) {
		if (lua_ref_isvalid(&obj->ref)) {
			lua_ref_push(L, &obj->ref);

			if (owner) {
				struct lua_ref tmp = lua_ref_init;
				assert(!obj->ref.weak);
				lua_ref_get(L, &tmp, -1, true);
				lua_ref_clear(L, &obj->ref);
				obj->ref = tmp;
			}

			return true;
		}
		else {
			return false;
		}
	}
	else {
		lua_pushnil(L);
		return true;
	}
}
