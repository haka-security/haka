
# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(libhaka lua SHARED
	packet.i
	)

# Lua dependency
LUA_LINK(libhaka)
swig_link_libraries(libhaka libhaka-c)

# Haka library shared between runtime and modules
add_library(libhaka-c SHARED
	packet.c
	log.c
	module.c
	error.c
	thread.c)

LUA_LINK(libhaka-c)
find_package(DL REQUIRED)
target_link_libraries(libhaka-c ${DL_LIBRARIES})
target_link_libraries(libhaka-c ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(libhaka-c PROPERTIES OUTPUT_NAME haka)

# Install
install(TARGETS libhaka-c LIBRARY DESTINATION lib)
