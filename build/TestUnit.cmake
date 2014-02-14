# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This test will run a Check unit test program build from the sources
# TEST_UNIT_FILES given as argument.

find_package(Check REQUIRED)

macro(TEST_UNIT)
	set(oneValueArgs MODULE NAME)
	set(multiValueArgs FILES ENV)
	cmake_parse_arguments(TEST_UNIT "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	add_executable(${TEST_UNIT_MODULE}-${TEST_UNIT_NAME} ${TEST_UNIT_FILES})
	target_link_libraries(${TEST_UNIT_MODULE}-${TEST_UNIT_NAME} ${CHECK_LIBRARIES})
	set_property(TARGET ${TEST_UNIT_MODULE}-${TEST_UNIT_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${CHECK_INCLUDE_DIRS})
	set_property(TARGET ${TEST_UNIT_MODULE}-${TEST_UNIT_NAME} APPEND PROPERTY LINK_FLAGS -Wl,--exclude-libs,ALL)
	add_test(${TEST_UNIT_MODULE}-${TEST_UNIT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_UNIT_MODULE}-${TEST_UNIT_NAME})
	list(APPEND TEST_UNIT_ENV "LD_LIBRARY_PATH=${LUA_LIBRARY_DIR}")
	list(APPEND TEST_UNIT_ENV "HAKA_PATH=${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}")
	set_tests_properties(${TEST_UNIT_MODULE}-${TEST_UNIT_NAME} PROPERTIES ENVIRONMENT "${TEST_UNIT_ENV}")
endmacro(TEST_UNIT)
