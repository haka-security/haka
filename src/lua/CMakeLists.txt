
# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-lua lua STATIC
	app.i
	module.i
	log.i
	packet.i
	stream.i
	state.c)

# Lua dependency
LUA_LINK(haka-lua)

install(FILES haka2.lua DESTINATION bin/modules)

include_directories(.)
