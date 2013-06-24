
# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-lua lua STATIC
	app.i
	module.i
	log.i
	stream.i
	state.c)

# Lua dependency
LUA_LINK(haka-lua)

install(FILES rule.lua DESTINATION share/haka/core)
install(FILES utils.lua DESTINATION share/haka/core)

include_directories(.)
