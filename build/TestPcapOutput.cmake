#
# This test will run an Haka script on a pcap file. The output (printed
# by the script into a text file) is then compared with a reference file.
# The test expects the following files :
#   * <name>.lua : The Haka script
#   * <name>.pcap : The source pcap file
#   * <name>-ref.txt : The reference pcap file
#

find_program(DIFF_COMMAND diff)
if(NOT DIFF_COMMAND)
	message(FATAL_ERROR "Cannot find diff command")
endif(NOT DIFF_COMMAND)

macro(TEST_PCAP_OUTPUT module name)
	add_test(NAME ${module}-${name}-pcap-output
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DLDDIR=${MODULES_BINARY_DIRS}:${CMAKE_BINARY_DIR}/out/bin/modules/*
		-DEXE=$<TARGET_FILE:haka>
		-DCONF=${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua
		-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/${name}.pcap
		-DREF=${CMAKE_CURRENT_SOURCE_DIR}/${name}-ref.txt
		-DDST=${name}-out.txt
		-DDIFF=${DIFF_COMMAND}
		-P ${CTEST_MODULE_DIR}/TestPcapOutputRun.cmake)
endmacro(TEST_PCAP_OUTPUT)
