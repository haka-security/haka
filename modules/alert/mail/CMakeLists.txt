# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(LibCurl)

option(LIB_CURL_SKIP_VERIFICATION "Skip SSL certificat verification" OFF)

if(LIBCURL_FOUND)
	add_library(alert-mail MODULE main.c)
	include_directories(${LIBCURL_INCLUDE_DIR})

	target_link_libraries(alert-mail LINK_PRIVATE ${LIBCURL_LIBRARY})

	set_target_properties(alert-mail PROPERTIES OUTPUT_NAME mail)
	INSTALL_MODULE(alert-mail alert)

	if (SKIP_VERIFICATION)
	    add_definitions(-DSKIP_PEER_VERIFICATION)
	    add_definitions(-DSKIP_HOSTNAME_VERIFICATION)
	endif ()
endif()
