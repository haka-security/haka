
# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-lua lua STATIC
	app.i
	module.i
	log.i
	packet.i
	state.c)

# Lua dependency
add_dependencies(haka-lua luajit)
include_directories(${LUA_INCLUDE_DIR})
target_link_libraries(haka-lua ${LUA_LIBRARY})


include_directories(.)
