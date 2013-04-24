
set(ENV{LD_LIBRARY_PATH} ${LDDIR})
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{DIFF} ${DIFF})
set(ENV{TSHARK} ${TSHARK})

execute_process(COMMAND ${EXE} ${CTEST_MODULE_DIR}/TestPcapCompare.lua ${CONF} ${SRC} ${DST} ${LDDIR}
	RESULT_VARIABLE HAD_ERROR)

execute_process(COMMAND bash ${CTEST_MODULE_DIR}/PrintEnviron.sh)

if(HAD_ERROR)
	message(FATAL_ERROR "Haka script failed")
endif(HAD_ERROR)

execute_process(COMMAND bash ${CTEST_MODULE_DIR}/ComparePcap.sh ${DST} ${REF}
	RESULT_VARIABLE HAD_ERROR)

if(HAD_ERROR)
	message(FATAL_ERROR "Pcap different")
endif(HAD_ERROR)
