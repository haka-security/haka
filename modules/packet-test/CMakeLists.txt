
add_library(packet-test MODULE main.c)
target_link_libraries(packet-test libhaka)

install(TARGETS packet-test LIBRARY DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(packet-test)

