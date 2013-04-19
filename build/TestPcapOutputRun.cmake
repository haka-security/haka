
set(ENV{LD_LIBRARY_PATH} ${LDDIR})
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})

execute_process(COMMAND ${EXE} ${CTEST_MODULE_DIR}/TestPcapOutput.lua ${CONF} ${SRC} ${DST}
	RESULT_VARIABLE HAD_ERROR)

if(HAD_ERROR)
	message(FATAL_ERROR "Haka script failed")
endif(HAD_ERROR)

execute_process(COMMAND ${DIFF} ${DST} ${REF}
	RESULT_VARIABLE HAD_ERROR)

if(HAD_ERROR)
	message(FATAL_ERROR "Output different")
endif(HAD_ERROR)
