# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)

add_custom_target(bench
	COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${CMAKE_CURRENT_SOURCE_DIR}/hakabench
		-DEXE_OPTIONS=${TEST_BENCH_OPTIONS}
		-DBENCH=${CMAKE_CURRENT_SOURCE_DIR}/benchmark
		-DHAKA_PATH=${TEST_RUNDIR}
		-DCONF=${CMAKE_BINARY_DIR}/test/etc/haka/haka.conf
		-P ${CTEST_MODULE_DIR}/TestBenchRun.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Run benchmark test" VERBATIM
	DEPENDS pretests
)

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
