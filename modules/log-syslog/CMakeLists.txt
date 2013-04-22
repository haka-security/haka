
add_library(log-syslog MODULE main.c)
INSTALL_MODULE(log-syslog)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(log-syslog)

