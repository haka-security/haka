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

find_program(VALGRIND_COMMAND valgrind)

macro(TEST_PCAP module name)
	set(cur "")
	set(TEST_PCAP_OPTIONS "")

	foreach(it ${ARGN})
		if(it STREQUAL "OPTIONS")
			set(cur "TEST_PCAP_OPTIONS")
		elseif(cur)
			set(${cur} "${${cur}} ${it}")
		endif()
	endforeach(it ${ARGN})
	
	add_test(NAME ${module}-${name}-pcap
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DCTEST_MODULE_BINARY_DIR=${CTEST_MODULE_BINARY_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=$<TARGET_FILE:haka>
		-DEXE_OPTIONS=${TEST_PCAP_OPTIONS}
		-DCONF=${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua
		-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/${name}.pcap
		-DREF=${CMAKE_CURRENT_SOURCE_DIR}/${name}-ref
		-DDST=${name}-out
		-DDIFF=${DIFF_COMMAND}
		-DTSHARK=${TSHARK_COMMAND}
		-DVALGRIND=${VALGRIND_COMMAND}
		-P ${CTEST_MODULE_DIR}/TestPcapRun.cmake)
endmacro(TEST_PCAP)

configure_file(${CTEST_MODULE_DIR}/TestPcap.lua.in ${CTEST_MODULE_BINARY_DIR}/TestPcap.lua)
