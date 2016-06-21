# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)

add_custom_target(bench
	COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${TEST_RUNDIR}/bin/hakabench
		-DBENCH=${CMAKE_CURRENT_SOURCE_DIR}/benchmark
		-DHAKA=${TEST_RUNDIR}/sbin/haka
		-DHAKA_PATH=${TEST_RUNDIR}
		-P ${CTEST_MODULE_DIR}/TestBenchRun.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Run benchmark test" VERBATIM
	DEPENDS pretests
)

add_custom_target(profiling
	COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${TEST_RUNDIR}/share/haka/scripts/hakabench
		-DEXE_OPTIONS=-vp
		-DBENCH=${CMAKE_CURRENT_SOURCE_DIR}/benchmark
		-DHAKA=${TEST_RUNDIR}/sbin/haka
		-DHAKA_PATH=${TEST_RUNDIR}
		-P ${CTEST_MODULE_DIR}/TestBenchRun.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Run benchmark test" VERBATIM
	DEPENDS pretests
)

install(PROGRAMS hakabench DESTINATION bin)

TEST_PCAP(core rule-error)
TEST_PCAP(core hakaevents)
TEST_PCAP(core timestamp)
TEST_PCAP(core state_machine)
TEST_PCAP(core payload)
TEST_PCAP(core alert)
TEST_PCAP(core stream-rule)
TEST_PCAP(core help OPTIONS --help)
TEST_PCAP(core token-grammar)
TEST_PCAP(core token-grammar-coroutine)
TEST_PCAP(core grammar-array-setter)
TEST_PCAP(core policy-ipv4)
TEST_PCAP(core dissector-parent)

TEST_UNIT_LUA(MODULE core NAME policy-criteria FILES policy-criteria.lua)
