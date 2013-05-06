
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(proto-ipv4 lua SHARED
	ipv4.i
	main.c
	ipv4.c)

include_directories(.)

target_link_libraries(proto-ipv4 libhaka)

# Lua dependency
LUA_LINK(proto-ipv4)

# Install
install(TARGETS proto-ipv4 LIBRARY DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(proto-ipv4)
add_subdirectory(test)
