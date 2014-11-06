/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUABINDING_H
#define _HAKA_LUABINDING_H

#include <lua.h>
#include <lauxlib.h>
#include <dlfcn.h>

#include <haka/lua/state.h>
#include <haka/lua/luautils.h>
#include <haka/config.h>

#define LUA_BIND_INIT(name) int luaopen_##name(lua_State *L)
#define LUA_LOAD(name, L) do {\
	lua_ffi_preload(L, lua_load_##name);\
	lua_load_##name(L);\
} while(0)

static inline void lua_ffi_preload(lua_State *L, void *addr)
{
#ifdef HAKA_FFI
	Dl_info info;
	LUA_STACK_MARK(L);

	dladdr(addr, &info);

	lua_state_require(L, "ffibinding", 1);
	lua_getfield(L, -1, "preload");
	lua_pushstring(L, info.dli_fname);
	lua_call(L, 1, 0);
	lua_pop(L, 1);

	LUA_STACK_CHECK(L, 0);
#endif /* HAKA_FFI */
}

#endif /* _HAKA_LUABINDING_H */
