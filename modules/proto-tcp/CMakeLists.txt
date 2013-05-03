
# Lua dependency
find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(proto-tcp lua SHARED
	tcp.i
	main.c
	tcp.c
	tcp-stream.c
	tcp-connection.c)

LUA_LINK(proto-tcp)

DEPENDS_MODULE(proto-tcp proto-ipv4)
INSTALL_MODULE(proto-tcp)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(proto-tcp)
add_subdirectory(test)
