
# Initialize
cmake_minimum_required(VERSION 2.6)
project(haka)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/build)

# Setup globals
set(CMAKE_INSTALL_PREFIX out)

include(DistClean)

include_directories(include)

# Add sub directories
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(modules)

