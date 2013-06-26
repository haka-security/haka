
# Initialize
cmake_minimum_required(VERSION 2.6)
include(TestBigEndian)
include(CheckTypeSize)

# Warn in source build
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(WARNING "In-source builds are not recommanded.")
endif()

project(haka)

# Version
include(version.cmake)

if(DEFINED BUILDTAG)
	set(HAKA_VERSION_BUILD ${BUILDTAG})
else()
	execute_process(COMMAND "date" "+%Y%m%d_%H%M" OUTPUT_VARIABLE BUILDTAG RESULT_VARIABLE HAD_ERROR)
	if(HAD_ERROR)
		message(FATAL_ERROR "failed to get current date")
	endif(HAD_ERROR)

	string(REGEX REPLACE "([^\n]+).*" "\\1" HAKA_VERSION_BUILD ${BUILDTAG})
endif()

message(STATUS "Build version: ${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_BUILD}")

# Build type
set(BUILD "None" CACHE STRING "Selected build type")
set(CMAKE_BUILD_TYPE ${BUILD})
message(STATUS "Building configuration (BUILD): ${CMAKE_BUILD_TYPE}")

# Install prefix
set(PREFIX "/usr/local" CACHE STRING "Installation prefix")
message(STATUS "Installation prefix (PREFIX): ${PREFIX}")

if(NOT IS_ABSOLUTE ${PREFIX})
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${PREFIX})
else()
	set(CMAKE_INSTALL_PREFIX ${PREFIX})
endif()

# Configure
set(CTEST_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CTEST_MODULE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build ${CTEST_MODULE_BINARY_DIR} ${CMAKE_MODULE_PATH})

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "-Wall -Werror")
endif()

find_package(Threads REQUIRED)

option(USE_LUAJIT "Use LuaJit library instead of Lua 5.1" YES)
set(HAKA_MODULE_PREFIX "" CACHE STRING "Haka module prefix")
set(HAKA_MODULE_SUFFIX ".ho" CACHE STRING "Haka module suffix")

TEST_BIG_ENDIAN(BIGENDIAN)
set(HAKA_BIGENDIAN ${BIGENDIAN})
if(BIGENDIAN EQUAL 0)
	set(HAKA_LITTLEENDIAN 1)
endif(BIGENDIAN EQUAL 0)

if(USE_LUAJIT)
	set(HAKA_LUAJIT 1)
else()
	set(HAKA_LUA 1)
endif()

# Find type size
CHECK_TYPE_SIZE("short" SIZEOF_SHORT)
CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("long long" SIZEOF_LONG_LONG)

# Search for 16 bits type
if(SIZEOF_SHORT EQUAL 2)
	SET(HAKA_16BIT_TYPE "short")
else(SIZEOF_SHORT EQUAL 2)
	if(SIZEOF_INT EQUAL 2)
		SET(HAKA_16BIT_TYPE "int")
	else(SIZEOF_INT EQUAL 2)
		MESSAGE(FATAL_ERROR "no 16 bit type found")
	endif(SIZEOF_INT EQUAL 2)
endif(SIZEOF_SHORT EQUAL 2)

# Search for 32 bits type
if(SIZEOF_INT EQUAL 4)
	SET(HAKA_32BIT_TYPE "int")
else(SIZEOF_INT EQUAL 4)
	if(SIZEOF_LONG EQUAL 4)
		SET(HAKA_32BIT_TYPE "long")
	else(SIZEOF_LONG EQUAL 4)
		MESSAGE(FATAL_ERROR "no 32 bit type found")
	endif(SIZEOF_LONG EQUAL 4)
endif(SIZEOF_INT EQUAL 4)

# Search for 64 bits type
if(SIZEOF_INT EQUAL 8)
	SET(HAKA_64BIT_TYPE "int")
else(SIZEOF_INT EQUAL 8)
	if(SIZEOF_LONG_LONG EQUAL 8)
		SET(HAKA_64BIT_TYPE "long long")
	else(SIZEOF_LONG_LONG EQUAL 8)
		MESSAGE(FATAL_ERROR "no 64 bit type found")
	endif(SIZEOF_LONG_LONG EQUAL 8)
endif(SIZEOF_INT EQUAL 8)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# Setup globals
include_directories(include)

# Tests
enable_testing()
find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})

# Add sub directories)
add_subdirectory(external)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(modules)
add_subdirectory(sample)

# Configuration files
configure_file(src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/haka/config.h)
configure_file(src/lua/config.lua.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.lua)

find_package(Sphinx)
if(SPHINX_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY)
	add_custom_target(doc
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/doc
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Sphinx" VERBATIM
	)
endif(SPHINX_FOUND)

# add a target to run cppcheck on the sources
find_package(Cppcheck)
if(CPPCHECK_FOUND)
	add_custom_target(cppcheck
		${CPPCHECK_EXECUTABLE} --enable=all -imake -iexternal -Iincludes -Imake/includes .
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Run cppcheck static analyzer on the sources" VERBATIM
	)
endif(CPPCHECK_FOUND)

# Packaging
set(CPACK_GENERATOR "STGZ")

set(CPACK_PACKAGE_VENDOR "Arkoon Network Security")
set(CPACK_PACKAGE_VERSION_MAJOR ${HAKA_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HAKA_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HAKA_VERSION_BUILD})

include(CPack)
