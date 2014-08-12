/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_OBJECT_H
#define _HAKA_LUA_OBJECT_H

#include <haka/types.h>

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

#define LUA_OBJECT_INIT        { NULL }
extern const struct lua_object lua_object_init;

void lua_object_initialize(struct lua_State *L);
bool lua_object_ownedbylua(struct lua_object *obj);
void lua_object_release(void *ptr, struct lua_object *obj);

#endif /* _HAKA_LUA_OBJECT_H */
