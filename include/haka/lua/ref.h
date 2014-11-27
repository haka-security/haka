/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_LUA_REF_H
#define HAKA_LUA_REF_H

#include <haka/types.h>


struct lua_state;
struct lua_State;


/*
 * Lua reference management
 */

struct lua_ref {
	struct lua_state *state;
	int               ref;
	bool              weak;
};

#define LUA_REF_INIT          { NULL, -1, false }
extern const struct lua_ref   lua_ref_init;

void lua_ref_init_state(struct lua_State *L);

bool lua_ref_isvalid(struct lua_ref *ref);
void lua_ref_get(struct lua_State *state, struct lua_ref *ref, int index, bool weak);
bool lua_ref_clear(struct lua_ref *ref);
void lua_ref_push(struct lua_State *state, struct lua_ref *ref);

struct lua_ref *lua_object_get_ref(void *_obj);

#endif /* HAKA_LUA_REF_H */
