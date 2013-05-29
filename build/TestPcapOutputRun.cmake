
set(ENV{LD_LIBRARY_PATH} ${LDDIR})
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})

execute_process(COMMAND ${EXE} ${CTEST_MODULE_DIR}/TestPcapOutput.lua ${CONF} ${SRC} ${LDDIR}
	RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp)

execute_process(COMMAND cat ${DST}-tmp)

if(HAD_ERROR)
	message(FATAL_ERROR "Haka script failed")
endif(HAD_ERROR)

# Filter out some initialization and exit messages
execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CompareOutput.awk ${DST}-tmp OUTPUT_FILE ${DST})

message(STATUS "Original output file is ${REF}")
message(STATUS "Generated output file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}")

message(STATUS "Diff output")

execute_process(COMMAND ${DIFF} ${DST} ${REF}
	RESULT_VARIABLE HAD_ERROR)

if(HAD_ERROR)
	message(FATAL_ERROR "Output different")
endif(HAD_ERROR)
