
add_library(log-syslog MODULE main.c)
target_link_libraries(log-syslog libhaka)

# Install
install(TARGETS log-syslog LIBRARY DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(log-syslog)

