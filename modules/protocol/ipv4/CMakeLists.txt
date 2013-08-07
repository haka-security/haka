
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

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(ipv4 protocol)
add_subdirectory(test)
