/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_LUA_OBJECT_H
#define HAKA_LUA_OBJECT_H

#include <haka/types.h>
#include <haka/lua/ref.h>


/*
 * Lua object management
 * This type allow to safely share an object between the C
 * and Lua by taking care of never keeping in Lua a reference
 * on a data that has been destroyed.
 */

struct lua_object {
	void            **owner;
	struct lua_ref    ref;
	bool              keep:1;
};

#define LUA_OBJECT_INIT        { NULL, ref: LUA_REF_INIT, keep: false }
extern const struct lua_object lua_object_init;

void lua_object_initialize(struct lua_State *L);
void lua_object_release(struct lua_object *obj);

struct lua_State;

void lua_object_register(struct lua_State *L, struct lua_object *obj, void **owner, int index, bool disown);
bool lua_object_push(struct lua_State *L, void *ptr, struct lua_object *obj, bool owner);

#ifdef HAKA_FFI
struct ffi_object {
	struct lua_ref *ref;
	void           *ptr;
};
#endif

#endif /* HAKA_LUA_OBJECT_H */
