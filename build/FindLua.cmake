# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindLua51)

find_program(LUA_COMPILER
	NAMES luac51 luac5.1 luac-5.1 luac
	HINTS $ENV{LUA_DIR}
	PATH_SUFFIXES bin
	DOC "Lua compiler"
)

set(LUA_FLAGS_NONE "")
set(LUA_FLAGS_DEBUG "")
set(LUA_FLAGS_MEMCHECK "")
set(LUA_FLAGS_RELEASE "-s")
set(LUA_FLAGS_RELWITHDEBINFO "")
set(LUA_FLAGS_MINSIZEREL "-s")
