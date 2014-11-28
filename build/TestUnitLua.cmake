# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# TEST_UNIT_LUA_FILES, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This test will run Lua unit test program
#

macro(TEST_UNIT_LUA)
	set(oneValueArgs MODULE NAME)
	set(multiValueArgs FILES ENV)
	cmake_parse_arguments(TEST_UNIT_LUA "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(CONF "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_UNIT_LUA_FILES}.lua")
	configure_file(${CTEST_MODULE_DIR}/unit.lua ${CMAKE_CURRENT_BINARY_DIR}/${TEST_UNIT_LUA_MODULE}-${TEST_UNIT_LUA_NAME}-unit.lua)

	add_test(NAME ${TEST_UNIT_LUA_MODULE}-${TEST_UNIT_LUA_NAME}-unit-lua
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${TEST_RUNDIR}/bin/hakapcap
		-DHAKA_PATH=${TEST_RUNDIR}
		-DHAKA_CCOMP_RUNTIME_DIR=${TEST_ROOT}/${HAKA_CCOMP_RUNTIME_DIR}
		-DCONF=${CMAKE_CURRENT_BINARY_DIR}/${TEST_UNIT_LUA_MODULE}-${TEST_UNIT_LUA_NAME}-unit.lua
		-DSRC=${CTEST_MODULE_DIR}/empty.pcap
		-DDST=${TEST_UNIT_LUA_NAME}-lua-out
		-DDIFF=${DIFF_COMMAND}
		-P ${CTEST_MODULE_DIR}/TestUnitLuaRun.cmake)
	set_tests_properties(${TEST_UNIT_LUA_MODULE}-${TEST_UNIT_LUA_NAME}-unit-lua PROPERTIES ENVIRONMENT "${TEST_UNIT_LUA_ENV}")
endmacro(TEST_UNIT_LUA)
