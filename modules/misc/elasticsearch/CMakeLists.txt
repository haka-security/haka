# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Jansson)
find_package(LibCurl)
find_package(LibUuid)

if(JANSSON_FOUND AND LIBCURL_FOUND AND LIBUUID_FOUND)
	swig_process(elasticsearchswig lua elasticsearch.i)

	add_library(libelasticsearch SHARED
		elasticsearch.c
		json.c
	)

	include_directories(${JANSSON_INCLUDE_DIR} ${LIBCURL_INCLUDE_DIR} ${LIBUUID_INCLUDE_DIR})
	target_link_libraries(libelasticsearch LINK_PRIVATE ${LIBCURL_LIBRARY} ${LIBUUID_LIBRARY} libhaka)
	target_link_libraries(libelasticsearch ${JANSSON_LIBRARY})
	set_target_properties(libelasticsearch PROPERTIES VERSION ${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}
		SOVERSION ${HAKA_VERSION_MAJOR})

	install(TARGETS libelasticsearch LIBRARY DESTINATION ${MODULE_INSTALL_LIB})

	add_library(elasticsearch MODULE
		main.c
		${SWIG_elasticsearchswig_FILES}
	)

	target_link_libraries(elasticsearch libelasticsearch)

	SWIG_FIX_ENTRYPOINT(elasticsearch misc)

	INSTALL_MODULE(elasticsearch misc)
else()
    message(STATUS "Not building module elasticsearch (missing libraries)")
endif()
