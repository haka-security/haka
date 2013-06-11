
INCLUDE_MODULE(ipv4)

find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(tcp lua SHARED
	tcp.i
	main.c
	tcp.c
	tcp-stream.c
	tcp-connection.c)

LUA_LINK(tcp)

DEPENDS_MODULE(tcp ipv4)
INSTALL_MODULE(tcp)

install(FILES tcp-connection.lua DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(tcp)
add_subdirectory(test)
