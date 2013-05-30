
set(ENV{LD_LIBRARY_PATH} ${LDDIR})
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{DIFF} ${DIFF})
set(ENV{TSHARK} ${TSHARK})

message(STATUS "Exporting var: export LD_LIBRARY_PATH=${LDDIR}")
message(STATUS "Executing ${EXE} ${CTEST_MODULE_DIR}/TestPcapCompare.lua ${CONF} ${SRC} ${DST} ${LDDIR}")
execute_process(COMMAND ${EXE} ${CTEST_MODULE_DIR}/TestPcapCompare.lua ${CONF} ${SRC} ${DST} ${LDDIR}
	RESULT_VARIABLE HAD_ERROR)

message(STATUS "All outputs and commands for this test are located in ${CMAKE_CURRENT_SOURCE_DIR}")

if(HAD_ERROR)
	message(FATAL_ERROR "Haka script failed")
endif(HAD_ERROR)

execute_process(COMMAND bash ${CTEST_MODULE_DIR}/ComparePcap.sh ${DST} ${REF}
	RESULT_VARIABLE HAD_ERROR)

if(HAD_ERROR)
	message(FATAL_ERROR "Pcap different")
endif(HAD_ERROR)
