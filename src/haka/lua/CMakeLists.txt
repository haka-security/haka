# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-liblua lua STATIC
	haka.i
	log.i
	alert.i
	packet.i
	state_machine.i
	state.c)

# Lua dependency
lua_link(haka-liblua)
target_link_libraries(haka-liblua libhaka)
target_link_libraries(haka-liblua libluadebug)

lua_compile(NAME haka-lua FILES
	rule.lua
	rule_group.lua
	color.lua
	class.lua
	events.lua
	context.lua
	state_machine.lua
	grammar.lua
	dissector.lua)
lua_compile(NAME haka-lua FILES interactive.lua FLAGS ${LUA_FLAGS_DEBUG})

lua_install(TARGET haka-lua DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/core)

include_directories(.)
