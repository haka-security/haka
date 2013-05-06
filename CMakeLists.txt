
# Initialize
cmake_minimum_required(VERSION 2.6)
include(TestBigEndian)
include(CheckTypeSize)

# Forbid in source build
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not allowed.")
endif()

project(haka)

# Configure
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build ${CMAKE_MODULE_PATH})
set(CTEST_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/haka/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# Setup globals
set(CMAKE_INSTALL_PREFIX out)

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
add_subdirectory(tests)

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
