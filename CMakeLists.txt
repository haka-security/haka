
# Initialize
cmake_minimum_required(VERSION 2.6)
project(haka)

# Setup globals
set(CMAKE_INSTALL_PREFIX out)

include_directories(include)

# Add sub directories
add_subdirectory(src)
add_subdirectory(modules)

