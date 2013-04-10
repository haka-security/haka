
# Initialize
cmake_minimum_required(VERSION 2.6)

# Forbid in source build
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not allowed.")
endif()

project(haka)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build ${CMAKE_MODULE_PATH})
set(CTEST_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "-Wall -Werror")
endif()

# Setup globals
set(CMAKE_INSTALL_PREFIX out)

include_directories(include)

enable_testing()

# Add sub directories
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(modules)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)

# Packaging
include(CPack)

