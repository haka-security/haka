/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_MARSHAL_H
#define _HAKA_LUA_MARSHAL_H

#include <haka/types.h>

struct lua_State;

char *lua_marshal(struct lua_State *L, int index, size_t *len);
bool lua_unmarshal(struct lua_State *L, const char *data, size_t len);

#endif /* _HAKA_LUA_MARSHAL_H */
