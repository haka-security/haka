
set(ENV{LANG} "C")
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)
set(ENV{HAKA_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX})
set(ENV{LD_LIBRARY_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/lib)

set(CMAKE_MODULE_PATH ${CTEST_MODULE_DIR} ${CMAKE_MODULE_PATH})
include(Valgrind)

message("Executing LANG=\"C\" LUA_PATH=\"$ENV{LUA_PATH}\" HAKA_PATH=\"$ENV{HAKA_PATH}\" LD_LIBRARY_PATH=\"$ENV{LD_LIBRARY_PATH}\" ${EXE} -d --no-pass-through ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}")

VALGRIND(${DST} ${EXE} -d --no-pass-through ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}
	RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)

execute_process(COMMAND cat ${DST}-tmp.txt OUTPUT_VARIABLE CONTENT)
message("${CONTENT}")

if(HAD_ERROR)
	message(FATAL_ERROR "Unit test failed")
endif(HAD_ERROR)

# Memory leak
CHECK_VALGRIND(${DST})
