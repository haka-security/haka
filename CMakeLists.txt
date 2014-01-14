# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Initialize
cmake_minimum_required(VERSION 2.8)
include(TestBigEndian)
include(CheckTypeSize)
include(ProcessorCount OPTIONAL)

if(COMMAND ProcessorCount)
	ProcessorCount(CPU_COUNT)
else()
	set(CPU_COUNT 1)
endif()

# Warn in source build
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(WARNING "In-source builds are not recommanded.")
endif()

project(haka NONE C)

set(CTEST_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CTEST_MODULE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build ${CTEST_MODULE_BINARY_DIR} ${CMAKE_MODULE_PATH})

# Version
include(version.cmake)
set(HAKA_VERSION_BUILD "")

# Add a build tag from the Git information
execute_process(COMMAND git symbolic-ref -q --short HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE BUILD_BRANCH RESULT_VARIABLE HAD_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git describe --dirty --always WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE BUILDTAG RESULT_VARIABLE HAD_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(HAD_ERROR EQUAL 0)
	if((BUILD_BRANCH MATCHES "^master$" OR BUILD_BRANCH MATCHES "^release/.*$") AND
		(NOT BUILDTAG MATCHES "-dirty"))
		# Release build not dirty, no extra buildtag
		execute_process(COMMAND rm -f "${CMAKE_SOURCE_DIR}/buildtag.cmake")
	else()
		set(HAKA_VERSION_BUILD "+${BUILDTAG}")
		execute_process(COMMAND echo "set(HAKA_VERSION_BUILD \"${HAKA_VERSION_BUILD}\")" OUTPUT_FILE ${CMAKE_SOURCE_DIR}/buildtag.cmake)
	endif()
else()
	include(buildtag.cmake OPTIONAL)
endif()

message(STATUS "Build version: ${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}${HAKA_VERSION_BUILD}")

# Distribution
find_package(Distrib REQUIRED)

# Build type
set(BUILD "Release" CACHE STRING "Selected build type")
set(CMAKE_BUILD_TYPE ${BUILD})
message(STATUS "Building configuration (BUILD): ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_MEMCHECK ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_MEMCHECK ${CMAKE_CXX_FLAGS_DEBUG})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(HAKA_DEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Memcheck")
	set(HAKA_DEBUG 1)
	set(HAKA_MEMCHECK 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(HAKA_NDEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(HAKA_NDEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
	set(HAKA_NDEBUG 1)
endif()

# Force the RUNPATH to be set to the RPATH to improve library search order
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")

# Default SWIG behavior to not create global module
find_package(SWIG 2 REQUIRED)
include(UseSWIG)

set(CMAKE_SWIG_FLAGS "-nomoduleglobal")

# Install prefix
set(PREFIX "/" CACHE STRING "Installation prefix")
message(STATUS "Installation prefix (PREFIX): ${PREFIX}")

if(NOT IS_ABSOLUTE ${PREFIX})
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${PREFIX})
else()
	set(CMAKE_INSTALL_PREFIX ${PREFIX})
endif()

set(HAKA_INSTALL_PREFIX "opt/haka")

if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/")
	set(CMAKE_INSTALL_PREFIX "")
	set(CMAKE_HAKA_INSTALL_PREFIX "/${HAKA_INSTALL_PREFIX}")
else()
	set(CMAKE_HAKA_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${HAKA_INSTALL_PREFIX}")
endif()

add_custom_target(localinstall
	COMMAND $(MAKE) install DESTDIR=${CMAKE_BINARY_DIR}/out
	COMMAND echo "export LD_LIBRARY_PATH=\"${CMAKE_CURRENT_BINARY_DIR}/out/opt/haka/lib\"; export HAKA_PATH=\"${CMAKE_CURRENT_BINARY_DIR}/out/opt/haka\"; export PATH=\"${CMAKE_CURRENT_BINARY_DIR}/out/opt/haka/bin\":\"${CMAKE_CURRENT_BINARY_DIR}/out/opt/haka/sbin\":$PATH" > ${CMAKE_CURRENT_BINARY_DIR}/out/env.sh
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Install locally..." VERBATIM
	VERBATIM
)

# Lua
set(LUA "luajit" CACHE STRING "Lua version")
message(STATUS "Lua version (LUA): ${LUA}")
include(UseLua)

# Configure
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
endif()

find_package(Threads REQUIRED)

set(HAKA_MODULE_PREFIX "" CACHE STRING "Haka module prefix")
set(HAKA_MODULE_SUFFIX ".ho" CACHE STRING "Haka module suffix")

TEST_BIG_ENDIAN(BIGENDIAN)
set(HAKA_BIGENDIAN ${BIGENDIAN})
if(BIGENDIAN EQUAL 0)
	set(HAKA_LITTLEENDIAN 1)
endif(BIGENDIAN EQUAL 0)

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
set(TEST_RUNDIR ${CMAKE_BINARY_DIR}/test/${CMAKE_INSTALL_PREFIX})

add_custom_target(pre-tests-build
	COMMAND $(MAKE) all
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	VERBATIM
)

add_custom_target(pre-tests
	COMMAND $(MAKE) install/fast DESTDIR=${CMAKE_BINARY_DIR}/test > /dev/null
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Preparing tests environment..." VERBATIM
	VERBATIM
	DEPENDS pre-tests-build
)

add_custom_target(tests
	COMMAND ctest -j${CPU_COUNT} $(CTEST_ARGS)
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Run all tests" VERBATIM
	DEPENDS pre-tests
)

# Add sub directories)
add_subdirectory(external)

include_directories(${UTHASH_INCLUDE})

add_subdirectory(doc)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(modules)
add_subdirectory(sample)
add_subdirectory(include)

# add a target to run cppcheck on the sources
find_package(Cppcheck)
if(CPPCHECK_FOUND)
	add_custom_target(cppcheck
		${CPPCHECK_EXECUTABLE} --enable=all -imake -iexternal -Iincludes -Imake/includes .
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Run cppcheck static analyzer on the sources" VERBATIM
	)
endif(CPPCHECK_FOUND)

include(Package)
