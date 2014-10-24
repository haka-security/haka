/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LUA_BIND_INIT(name) int luaopen_##name(lua_State *L)
#define LUA_LOAD(name, L) lua_load_##name(L);

#ifdef HAKA_FFI


#else /* HAKA_FFI */


#endif
