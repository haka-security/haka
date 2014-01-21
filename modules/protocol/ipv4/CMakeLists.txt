# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

swig_add_module(ipv4 lua SHARED
	ipv4.i
	main.c
	ipv4.c
	ipv4-addr.c
	ipv4-network.c)
SWIG_FIX_ENTRYPOINT(ipv4 protocol)

include_directories(.)

# Lua dependency
LUA_LINK(ipv4)

INSTALL_MODULE(ipv4 protocol)

install(DIRECTORY haka DESTINATION ${HAKA_INSTALL_PREFIX}/include FILES_MATCHING PATTERN "*.h" PATTERN "*.si")

# Tests
add_subdirectory(test)
