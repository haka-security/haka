# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

INCLUDE_MODULE(ipv4 protocol)

swig_add_module(tcp lua SHARED
	tcp.i
	main.c
	tcp.c
	tcp-stream.c
)
SWIG_FIX_ENTRYPOINT(tcp protocol)

lua_compile(NAME tcp-console FILES tcp_console.lua)
lua_install(TARGET tcp-console DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/console)

LUA_LINK(tcp)

DEPENDS_MODULE(tcp ipv4 protocol)
INSTALL_MODULE(tcp protocol)

lua_compile(NAME tcp_connection FILES tcp_connection.lua)
lua_install(TARGET tcp_connection DESTINATION ${MODULE_INSTALL_PATH}/protocol)

install(DIRECTORY haka DESTINATION ${HAKA_INSTALL_PREFIX}/include FILES_MATCHING PATTERN "*.h" PATTERN "*.si")

# Tests
add_subdirectory(test)
