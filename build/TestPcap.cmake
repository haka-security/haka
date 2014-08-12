# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This test will run an Haka script on a pcap file. The output pcap
# is then compared with a reference pcap file as well as the stdout
# output
# The test expects the following files :
#   * <name>.lua : The Haka script
#   * <name>.pcap : The source pcap file
#   * <name>-ref.pcap : The reference pcap file
#   * <name>-ref.txt : The reference output file
#

find_program(DIFF_COMMAND diff)
if(NOT DIFF_COMMAND)
	message(FATAL_ERROR "Cannot find diff command")
endif(NOT DIFF_COMMAND)

find_program(TSHARK_COMMAND tshark)
if(NOT TSHARK_COMMAND)
	message(FATAL_ERROR "Cannot find tshark command")
endif(NOT TSHARK_COMMAND)

find_program(GAWK_COMMAND gawk)
if (NOT GAWK_COMMAND)
	message(FATAL_ERROR "Cannot find gawk command")
endif(NOT GAWK_COMMAND)

macro(TEST_PCAP module name)
	set(oneValueArgs OPTIONS CONFIG)
	cmake_parse_arguments(TEST_PCAP "" "${oneValueArgs}" "" ${ARGN})

	if(TEST_PCAP_CONFIG)
	else()
		set(TEST_PCAP_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua)
	endif()

	add_test(NAME ${module}-${name}-pcap
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${TEST_RUNDIR}/bin/hakapcap
		-DEXE_OPTIONS=${TEST_PCAP_OPTIONS}
		-DHAKA_PATH=${TEST_RUNDIR}
		-DCONF=${TEST_PCAP_CONFIG}
		-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/${name}.pcap
		-DREF=${CMAKE_CURRENT_SOURCE_DIR}/${name}-ref
		-DREFVER=${CMAKE_LUA}
		-DDST=${name}-out
		-DDIFF=${DIFF_COMMAND}
		-DTSHARK=${TSHARK_COMMAND}
		-P ${CTEST_MODULE_DIR}/TestPcapRun.cmake)
endmacro(TEST_PCAP)
