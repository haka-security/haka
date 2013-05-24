#
# This test will run an Haka script on a pcap file. The output pcap
# is then compared with a reference pcap file.
# The test expects the following files :
#   * <name>.lua : The Haka script
#   * <name>.pcap : The source pcap file
#   * <name>-ref.pcap : The reference pcap file
#

find_program(DIFF_COMMAND diff)
if(NOT DIFF_COMMAND)
	message(FATAL_ERROR "Cannot find diff command")
endif(NOT DIFF_COMMAND)

find_program(TSHARK_COMMAND tshark)
if(NOT TSHARK_COMMAND)
	message(FATAL_ERROR "Cannot find tshark command")
endif(NOT TSHARK_COMMAND)

macro(TEST_PCAP_COMPARE module name)
	add_test(NAME ${module}-${name}-pcap-compare
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DLDDIR=${MODULES_BINARY_DIRS}
		-DEXE=$<TARGET_FILE:haka>
		-DCONF=${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua
		-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/${name}.pcap
		-DREF=${CMAKE_CURRENT_SOURCE_DIR}/${name}-ref.pcap
		-DDST=${name}-out.pcap
		-DDIFF=${DIFF_COMMAND}
		-DTSHARK=${TSHARK_COMMAND}
		-P ${CTEST_MODULE_DIR}/TestPcapCompareRun.cmake)
endmacro(TEST_PCAP_COMPARE)
