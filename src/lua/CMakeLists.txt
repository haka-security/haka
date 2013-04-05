
# Lua dependency
find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-lua lua STATIC
	app.i
	module.i
    log.i
    packet.i
	state.c)
#set_target_properties(libhaka-lua PROPERTIES COMPILE_FLAGS -fPIC)

target_link_libraries(haka-lua ${LUA_LIBRARY})

include_directories(.)
