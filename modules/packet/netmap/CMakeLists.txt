# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_REQUIRED_INCLUDES /usr/include)
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("net/netmap_user.h;net/netmap.h" HAVE_NETMAP)

if(HAVE_NETMAP)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_NETMAP ")
        add_library(packet-netmap MODULE main.c libnetmap.c)
	set_target_properties(packet-netmap PROPERTIES OUTPUT_NAME netmap)

	include_directories(.)

	INSTALL_MODULE(packet-netmap packet)
endif()
