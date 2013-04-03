
# Initialize
cmake_minimum_required(VERSION 2.6)
project(haka)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build ${CMAKE_MODULE_PATH})

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "-Werror")
endif()

# Setup globals
set(CMAKE_INSTALL_PREFIX out)

include(DistClean)

include_directories(include)

# Add sub directories
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(modules)

