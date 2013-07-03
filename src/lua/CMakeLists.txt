
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

install(FILES rule.lua utils.lua color.lua
	DESTINATION share/haka/core)

install(FILES color.lua
	DESTINATION share/haka/core/haka)

include_directories(.)
