# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This test will run Lua unit test program
#

macro(TEST_UNIT_LUA module name)
	set(CONF "${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua")
	configure_file(${CTEST_MODULE_DIR}/unit.lua ${CMAKE_CURRENT_BINARY_DIR}/${module}-${name}-unit.lua)

	add_test(NAME ${module}-${name}-unit-lua
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DCTEST_MODULE_BINARY_DIR=${CTEST_MODULE_BINARY_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/bin/hakapcap
		-DTEST_RUNDIR=${TEST_RUNDIR}
		-DHAKA_INSTALL_PREFIX=${HAKA_INSTALL_PREFIX}
		-DCONF=${CMAKE_CURRENT_BINARY_DIR}/${module}-${name}-unit.lua
		-DSRC=${CTEST_MODULE_DIR}/empty.pcap
		-DDST=${name}-out
		-DDIFF=${DIFF_COMMAND}
		-DVALGRIND=${VALGRIND_COMMAND}
		-P ${CTEST_MODULE_DIR}/TestUnitLuaRun.cmake)
endmacro(TEST_UNIT_LUA)
