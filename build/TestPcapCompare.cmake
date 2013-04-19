
set(CMAKE_TEMP_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)

macro(TEST_PCAP_COMPARE name)
	add_test(NAME pcap-compare-${name}
		COMMAND ${CMAKE_COMMAND}
		-DCTEST_MODULE_DIR=${CTEST_MODULE_DIR}
		-DLDDIR=$<TARGET_FILE_DIR:packet-pcap>
		-DEXE=$<TARGET_FILE:haka>
		-DCONF=${CMAKE_CURRENT_SOURCE_DIR}/${name}.lua
		-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/${name}.pcap
		-DREF=${CMAKE_CURRENT_SOURCE_DIR}/${name}-ref.pcap
		-DDST=${CMAKE_TEMP_DIR}/${name}-out.pcap
		-P ${CTEST_MODULE_DIR}/TestPcapCompareRun.cmake)
endmacro(TEST_PCAP_COMPARE)
