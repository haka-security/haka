
# Lua dependency
find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(proto-ipv4 lua MODULE
	ipv4.i
	main.c
	ipv4.c)

include_directories(.)

target_link_libraries(proto-ipv4 libhaka)
target_link_libraries(proto-ipv4 ${LUA_LIBRARY})

# Install
install(TARGETS proto-ipv4 LIBRARY DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(proto-ipv4)
