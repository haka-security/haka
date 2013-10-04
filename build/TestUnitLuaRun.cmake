
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)
set(ENV{HAKA_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX})
set(ENV{LD_LIBRARY_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/lib)

message("Executing LUA_PATH=\"$ENV{LUA_PATH}\" HAKA_PATH=\"$ENV{HAKA_PATH}\" LD_LIBRARY_PATH=\"$ENV{LD_LIBRARY_PATH}\" ${EXE} -d ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}")

if(VALGRIND AND NOT "$ENV{QUICK}" STREQUAL "yes")
	set(DO_VALGRIND 1)
endif()

if(DO_VALGRIND)
	execute_process(COMMAND ${VALGRIND} --log-file=${DST}-valgrind.txt ${EXE} -d ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}
		RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)
else()
	execute_process(COMMAND ${EXE} -d ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}
		RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)
endif()

execute_process(COMMAND cat ${DST}-tmp.txt OUTPUT_VARIABLE CONTENT)
message("${CONTENT}")

if(HAD_ERROR)
	message(FATAL_ERROR "Unit test failed")
endif(HAD_ERROR)

# Memory leak
if(DO_VALGRIND)
	message("")
	message("-- Memory leak check")
	execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CheckValgrind.awk ${DST}-valgrind.txt OUTPUT_VARIABLE VALGRIND_OUT)
	list(GET VALGRIND_OUT 0 VALGRIND_LEAK)
	list(GET VALGRIND_OUT 1 VALGRIND_REACHABLE)

	if(VALGRIND_LEAK GREATER 0)
		message(FATAL_ERROR "Memory leak detected: ${VALGRIND_LEAK} lost bytes")
	endif()

	if(VALGRIND_REACHABLE GREATER 32)
		message(FATAL_ERROR "Memory leak detected: ${VALGRIND_REACHABLE} reachable bytes")
	endif()

	message("No leak detected")
endif()
