/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_OBJECT_H
#define _HAKA_LUA_OBJECT_H

struct lua_state;
struct lua_State;


/*
 * Lua object management
 * This type allow to safely share an object between the C
 * and Lua by taking care of never keeping in Lua a reference
 * on a userdata that has been destroyed.
 */

struct lua_object {
	struct lua_state *state;
};

void lua_object_initialize(struct lua_State *L);
void lua_object_init(struct lua_object *obj);
void lua_object_release(void *ptr, struct lua_object *obj);

#endif /* _HAKA_LUA_OBJECT_H */
