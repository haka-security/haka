# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Build hakactl program
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(libhakactl STATIC
	ctl_comm.c)

swig_process(hakactlswig lua
	lua/hakactl.i
)

add_executable(hakactl
	hakactl.c
	commands.c
	console.c
	${SWIG_hakactlswig_FILES}
)

target_link_libraries(hakactl libhaka)
target_link_libraries(hakactl libhakactl)
target_link_libraries(hakactl ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS hakactl RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/sbin)

lua_compile(NAME hakactl-lua FILES
	lua/thread.lua
	lua/event.lua
	lua/rule.lua
	lua/misc.lua
)
lua_install(TARGET hakactl-lua DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/console)

install(FILES hakactl.1 DESTINATION ${HAKA_INSTALL_PREFIX}/share/man/man1)
