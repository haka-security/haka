
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(ipv4 lua SHARED
	ipv4.i
	main.c
	ipv4.c
	ipv4-addr.c
	ipv4-network.c)
SWIG_FIX_ENTRYPOINT(ipv4 protocol)

include_directories(.)

# Lua dependency
LUA_LINK(ipv4)
target_link_libraries(ipv4 libhaka)

INSTALL_MODULE(ipv4 protocol)

lua_compile(NAME ipv4lua FILES ipv4lua.lua)
lua_install(TARGET ipv4lua DESTINATION ${MODULE_INSTALL_PATH}/protocol)

install(DIRECTORY haka DESTINATION ${HAKA_INSTALL_PREFIX}/include FILES_MATCHING PATTERN "*.h" PATTERN "*.si")

# Tests
add_subdirectory(test)
