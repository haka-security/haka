/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_LUA_H
#define _HAKA_LUA_LUA_H

#include <assert.h>


#ifndef NDEBUG

#define LUA_STACK_MARK(L) \
	const int __stackmark = lua_gettop((L))

#define LUA_STACK_CHECK(L, offset) \
	assert(lua_gettop((L)) == __stackmark+(offset))

#else

#define LUA_STACK_MARK(L)
#define LUA_STACK_CHECK(L, offset)

#endif


const char *lua_converttostring(struct lua_State *L, int idx, size_t *len);

#endif /* _HAKA_LUA_LUA_H */
