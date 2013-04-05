
add_library(log-stdout MODULE main.c)
target_link_libraries(log-stdout libhaka)

option(USE_COLORS "Use colored logging for log-stdout module" YES)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Install
install(TARGETS log-stdout LIBRARY DESTINATION bin/modules)

# Tests
include(TestModuleLoad)
TEST_MODULE_LOAD(log-stdout)

