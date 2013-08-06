
add_library(log-stdout MODULE main.c)
set_target_properties(log-stdout PROPERTIES OUTPUT_NAME stdout)

option(USE_COLORS "Use colored logging for log-stdout module" YES)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

INSTALL_MODULE(log-stdout log)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(log-stdout)
