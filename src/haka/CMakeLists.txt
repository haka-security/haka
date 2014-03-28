# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(.)
include_directories(../hakactl)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# SWIG lua bindings for the lib haka
swig_process(hakalua lua
	lua/haka.i
	lua/log.i
	lua/alert.i
	lua/packet.i
	lua/state_machine.i
)

lua_compile(NAME haka-lua FILES
	lua/rule.lua
	lua/rule_group.lua
	lua/color.lua
	lua/class.lua
	lua/events.lua
	lua/context.lua
	lua/state_machine.lua
	lua/grammar.lua
	lua/utils.lua
	lua/dissector.lua
)
lua_compile(NAME haka-lua FILES lua/interactive.lua FLAGS ${LUA_FLAGS_DEBUG})
lua_install(TARGET haka-lua DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/core)

# Build common sources
add_library(haka-common STATIC
	app.c
	thread.c
	lua/state.c
	${SWIG_hakalua_FILES}
)

LUA_DEPENDS(haka-common)
target_link_libraries(haka-common libhaka)

# Build haka program
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_executable(haka
	haka.c
	ctl.c)

target_link_libraries(haka haka-common)
target_link_libraries(haka libhakactl)
target_link_libraries(haka ${CMAKE_THREAD_LIBS_INIT})
LUA_LINK(haka)

install(TARGETS haka RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/sbin)

configure_file(haka.conf.in ${CMAKE_CURRENT_BINARY_DIR}/haka.conf)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/haka.conf DESTINATION ${HAKA_INSTALL_PREFIX}/etc/haka)

install(FILES haka.1 hakapcap.1 DESTINATION ${HAKA_INSTALL_PREFIX}/share/man/man1)

# Build hakapcap program
add_executable(hakapcap
	hakapcap.c)

target_link_libraries(hakapcap haka-common)
target_link_libraries(hakapcap ${CMAKE_THREAD_LIBS_INIT})
LUA_LINK(hakapcap)

install(TARGETS hakapcap RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/bin)

add_subdirectory(init)
add_subdirectory(test)
