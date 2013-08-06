
add_library(log-syslog MODULE main.c)
set_target_properties(log-stdout PROPERTIES OUTPUT_NAME syslog)
INSTALL_MODULE(log-syslog log)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(log-syslog)
