
add_library(log-stdout MODULE main.c)
target_link_libraries(log-stdout libhaka)

install(TARGETS log-stdout LIBRARY DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(log-stdout)

