
INCLUDE_MODULE(ipv4 protocol)

find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(tcp lua SHARED
	tcp.i
	main.c
	tcp.c
	tcp-stream.c
	tcp-connection.c)

LUA_LINK(tcp)

DEPENDS_MODULE(tcp ipv4 protocol)
INSTALL_MODULE(tcp protocol)

lua_install(NAME tcp-connection FILES tcp-connection.lua DESTINATION ${MODULE_INSTALL_PATH}/protocol COMPILED)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(tcp)
add_subdirectory(test)
