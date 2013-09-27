
add_subdirectory(lua)

include_directories(${LIBINIPARSER_INCLUDE})

# Haka library shared between runtime and modules
add_library(libhaka SHARED
	packet.c
	log.c
	log_module.c
	module.c
	error.c
	thread.c
	parameters.c
	colors.c
	time.c
	timer.c
	state_machine.c
	container/list.c
	container/vector.c)

LUA_LINK(libhaka)
find_package(DL REQUIRED)
target_link_libraries(libhaka ${DL_LIBRARIES})
target_link_libraries(libhaka ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(libhaka libhaka-lua)
target_link_libraries(libhaka libiniparser)
target_link_libraries(libhaka rt)
target_link_libraries(libhaka m)

set_target_properties(libhaka PROPERTIES OUTPUT_NAME haka)

# Install
install(TARGETS libhaka LIBRARY DESTINATION ${HAKA_INSTALL_PREFIX}/lib)
