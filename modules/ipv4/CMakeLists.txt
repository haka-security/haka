
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(ipv4 lua SHARED
	ipv4.i
	main.c
	ipv4.c
	ipv4-addr.c
	ipv4-network.c)

include_directories(.)

target_link_libraries(ipv4 libhaka)

# Lua dependency
LUA_LINK(ipv4)

INSTALL_MODULE(ipv4)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(ipv4)
add_subdirectory(test)
