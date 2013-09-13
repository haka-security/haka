
# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-liblua lua STATIC
	app.i
	module.i
	log.i
	stream.i
	state.c)

# Lua dependency
lua_link(haka-liblua)
target_link_libraries(haka-liblua libhaka)

lua_compile(NAME haka-lua FILES rule.lua utils.lua color.lua)
lua_compile(NAME haka-lua FILES interactive.lua FLAGS ${LUA_FLAGS_DEBUG})

lua_install(TARGET haka-lua DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/core)

include_directories(.)
