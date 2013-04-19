
macro(TEST_PCAP_OUTPUT name)
	add_test(NAME pcap-output-${name}
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DLDDIR=$<TARGET_FILE_DIR:packet-pcap>
		-DEXE=$<TARGET_FILE:haka>
		-DCONF=${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua
		-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/${name}.pcap
		-DREF=${CMAKE_CURRENT_SOURCE_DIR}/${name}-ref.txt
		-DDST=${name}-out.txt
		-P ${CTEST_MODULE_DIR}/TestPcapOutputRun.cmake)
endmacro(TEST_PCAP_OUTPUT)
