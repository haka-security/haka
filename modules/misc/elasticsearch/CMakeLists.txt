# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Jansson)
find_package(LibCurl)
find_package(LibUuid)

if(JANSSON_FOUND AND LIBCURL_FOUND AND LIBUUID_FOUND)
	swig_process(elasticsearchswig lua elasticsearch.i)

	add_library(elasticsearch SHARED
		main.c
		elasticsearch.c
		json.c
		${SWIG_elasticsearchswig_FILES}
	)

	include_directories(${JANSSON_INCLUDE_DIR} ${LIBCURL_INCLUDE_DIR} ${LIBUUID_INCLUDE_DIR})
	target_link_libraries(elasticsearch ${JANSSON_LIBRARY} ${LIBCURL_LIBRARY} ${LIBUUID_LIBRARY})

	SWIG_FIX_ENTRYPOINT(elasticsearch misc)

	INSTALL_MODULE(elasticsearch misc)
else()
    message(STATUS "Not building module elasticsearch (missing libraries)")
endif()
