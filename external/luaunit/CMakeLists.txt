# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(LUAUNIT_FOUND luaunit.lua HINTS ${CMAKE_CURRENT_SOURCE_DIR}/src/)


if(NOT LUAUNIT_FOUND STREQUAL "LUAUNIT_FOUND-NOTFOUND")
	lua_compile(NAME luaunit FILES src/luaunit.lua)
	lua_install(TARGET luaunit DESTINATION share/haka/core)
endif()
