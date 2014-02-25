# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(${LIBINIPARSER_INCLUDE})

# Haka library shared between runtime and modules

swig_process(libhakalua lua lua/object.i)

add_library(libhaka SHARED
	packet.c
	log.c
	log_module.c
	alert.c
	alert_module.c
	module.c
	error.c
	debug.c
	thread.c
	parameters.c
	colors.c
	time.c
	stat.c
	timer.c
	state_machine.c
	vbuffer.c
	vbuffer_data.c
	vbuffer_stream.c
	regexp_module.c
	container/list.c
	container/list2.c
	container/vector.c
	lua/state.c
	lua/ref.c
	lua/lua.c
	${SWIG_libhakalua_FILES}
)

set_target_properties(libhaka PROPERTIES VERSION ${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}
	SOVERSION ${HAKA_VERSION_MAJOR})

LUA_LINK(libhaka)
find_package(DL REQUIRED)
target_link_libraries(libhaka ${DL_LIBRARIES})
target_link_libraries(libhaka ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(libhaka libiniparser)
target_link_libraries(libhaka rt)
target_link_libraries(libhaka m)

set_target_properties(libhaka PROPERTIES OUTPUT_NAME haka)

# Install
install(TARGETS libhaka LIBRARY DESTINATION ${HAKA_INSTALL_PREFIX}/lib)

add_subdirectory(test)
