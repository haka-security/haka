
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
target_link_libraries(haka-lua libhaka)

lua_install(NAME haka-lua-src FILES rule.lua utils.lua color.lua DESTINATION share/haka/core COMPILED)

include_directories(.)
