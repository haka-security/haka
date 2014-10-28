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

# System
message(STATUS "Build system: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")

# Version
include(version.cmake)
set(HAKA_VERSION_BUILD "")

set(GITTAG "yes" CACHE STRING "Append git tag to build number")
if(GITTAG STREQUAL "yes")
	# Add a build tag from the Git information
	execute_process(COMMAND git symbolic-ref -q --short HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE BUILD_BRANCH RESULT_VARIABLE HAD_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	execute_process(COMMAND git describe --dirty --always WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE BUILDTAG RESULT_VARIABLE HAD_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	if(HAD_ERROR EQUAL 0)
		set(HAKA_VERSION_BUILD "+${BUILDTAG}")
		execute_process(COMMAND echo "set(HAKA_VERSION_BUILD \"${HAKA_VERSION_BUILD}\")"
			OUTPUT_FILE ${CMAKE_SOURCE_DIR}/buildtag.cmake)
	else()
		include(buildtag.cmake OPTIONAL)
	endif()
endif()

set(HAKA_VERSION "${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}${HAKA_VERSION_BUILD}")
message(STATUS "Build version: ${HAKA_VERSION}")

# Build type
set(BUILD "Release" CACHE STRING "Selected build type")

if(NOT ("$ENV{BUILD}" STREQUAL ""))
	set(CMAKE_BUILD_TYPE $ENV{BUILD})
else()
	set(CMAKE_BUILD_TYPE ${BUILD})
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-gnu-designator")
endif()

set(CMAKE_C_FLAGS_MEMCHECK ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_MEMCHECK ${CMAKE_CXX_FLAGS_DEBUG})

set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage")
set(CMAKE_LINK_FLAGS_COVERAGE "${CMAKE_LINK_FLAGS_COVERAGE} --coverage")
set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} --coverage")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_FORTIFY_SOURCE=2 -fstack-protector --param ssp-buffer-size=4")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_FORTIFY_SOURCE=2 -fstack-protector --param ssp-buffer-size=4")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,-z,relro -Wl,-z,now")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-z,relro -Wl,-z,now")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-z,relro -Wl,-z,now")

# The RelWithDebInfo options should be the same as Release
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(HAKA_DEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
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
else()
	message(FATAL_ERROR "Build configuration (BUILD): Unsupported build ${CMAKE_BUILD_TYPE}")
endif()

message(STATUS "Build configuration (BUILD): ${CMAKE_BUILD_TYPE}")

# Coverage
find_package(LCov)
if(LCOV_FOUND)
	add_custom_target(coverage_clean
		COMMAND lcov -z --directory ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND rm -f coverage/coverage.info
		COMMENT "Clean coverage data" VERBATIM
	)

	add_custom_target(coverage
		COMMAND mkdir -p coverage
		COMMAND lcov --capture --directory ${CMAKE_CURRENT_BINARY_DIR} --output-file coverage/coverage_full.info
		COMMAND lcov -r coverage/coverage_full.info "*test*.c" -o coverage/coverage.info
		COMMAND lcov -r coverage/coverage.info "*LUA_wrap.c" -o coverage/coverage.info
		COMMAND lcov -r coverage/coverage.info "*external*" -o coverage/coverage.info
		COMMAND genhtml --legend coverage/coverage.info --output-directory coverage
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Build coverage data" VERBATIM
	)
endif(LCOV_FOUND)

# Force the RUNPATH to be set to the RPATH to improve library search order
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-new-dtags")

# Default SWIG behavior to not create global module
find_package(SWIG 2 REQUIRED)
include(UseSWIG)

set(CMAKE_SWIG_FLAGS "-nomoduleglobal")

# Install prefix
message(STATUS "Installation prefix (CMAKE_INSTALL_PREFIX): ${CMAKE_INSTALL_PREFIX}")
set(LOCALINSTALL "${CMAKE_BINARY_DIR}/out" CACHE STRING "Default localinstall dest")

set(ENV_SH ${LOCALINSTALL}/env.sh)

add_custom_target(localinstall
	COMMAND $(MAKE) install DESTDIR=${LOCALINSTALL}
	COMMAND echo "export LD_LIBRARY_PATH=\"${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/lib:${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/lib/haka:${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/lib/haka/modules/protocol:${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/lib/haka/modules/packet:${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/lib/haka/modules/misc\"" > ${ENV_SH}
	COMMAND echo "export HAKA_PATH=\"${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}\"" >> ${ENV_SH}
	COMMAND echo "export PATH=\"${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/bin\":\"${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/sbin\":\"${LOCALINSTALL}${CMAKE_INSTALL_PREFIX}/share/haka/scripts/\":$PATH" >> ${ENV_SH}
	COMMAND echo "export LUA_PATH=\"\$(realpath \$HAKA_PATH/share/haka/lua/share/luajit* | head -n 1)/?.lua\"" >> ${ENV_SH}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Install locally..." VERBATIM
	VERBATIM
)

# Lua
set(LUA "luajit" CACHE STRING "Lua version")

if(NOT ("$ENV{LUA}" STREQUAL ""))
	set(CMAKE_LUA $ENV{LUA})
else()
	set(CMAKE_LUA ${LUA})
endif()

message(STATUS "Lua version (LUA): ${CMAKE_LUA}")
include(UseLua)
include(UseBc2c)

# Lua binding
set(LUA_BINDING "ffi" CACHE STRING "Lua binding method")
if (CMAKE_LUA STREQUAL "lua")
	set(LUA_BINDING "cfunc")
endif()

if(NOT ("$ENV{LUA_BINDING}" STREQUAL ""))
	set(CMAKE_LUA_BINDING $ENV{LUA_BINDING})
else()
	set(CMAKE_LUA_BINDING ${LUA_BINDING})
endif()

if(NOT (${CMAKE_LUA_BINDING} STREQUAL "ffi"))
	set(HAKA_FFI 0)
	set(HAKA_CFUNC 1)
else()
	set(HAKA_FFI 1)
	set(HAKA_CFUNC 0)
endif()


message(STATUS "Lua binding method (LUA_BINDING): ${CMAKE_LUA_BINDING}")

find_package(Threads REQUIRED)

set(HAKA_MODULE_PREFIX "" CACHE STRING "Haka module prefix")
set(HAKA_MODULE_SUFFIX ".so" CACHE STRING "Haka module suffix")

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

add_custom_target(pretests-build
	COMMAND $(MAKE) all
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	VERBATIM
)

add_custom_target(pretests
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/test
	COMMAND $(MAKE) install/fast DESTDIR=${CMAKE_BINARY_DIR}/test > /dev/null
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Preparing tests environment..." VERBATIM
	VERBATIM
	DEPENDS pretests-build
)

add_custom_target(tests
	COMMAND ctest -j${CPU_COUNT} $(CTEST_ARGS)
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Run all tests" VERBATIM
	DEPENDS pretests
)

# Add sub directories)
add_subdirectory(external)

include_directories(${UTHASH_INCLUDE})

add_subdirectory(include)
add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(modules)
add_subdirectory(lib)
add_subdirectory(sample)

# add a target to run cppcheck on the sources
find_package(Cppcheck)
if(CPPCHECK_FOUND)
	add_custom_target(cppcheck
		${CPPCHECK_EXECUTABLE} --enable=all -imake -iexternal -Iincludes -Imake/includes .
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Run cppcheck static analyzer on the sources" VERBATIM
	)
endif(CPPCHECK_FOUND)

include(Package)
