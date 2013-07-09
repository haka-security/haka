
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{DIFF} ${DIFF})
set(ENV{TSHARK} ${TSHARK})
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)

message(STATUS "Executing LUA_PATH=\"${PROJECT_SOURCE_DIR}/src/lua/?.lua\" ${EXE} -d ${CTEST_MODULE_BINARY_DIR}/TestPcap.lua ${CONF} ${SRC} ${DST}.pcap")
execute_process(COMMAND ${EXE} -d ${CTEST_MODULE_BINARY_DIR}/TestPcap.lua ${CONF} ${SRC} ${DST}.pcap
	RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)

execute_process(COMMAND cat ${DST}-tmp.txt)

if(HAD_ERROR)
	message(FATAL_ERROR "Haka script failed")
endif(HAD_ERROR)

# Filter out some initialization and exit messages
execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CompareOutput.awk ${DST}-tmp.txt OUTPUT_FILE ${DST}.txt)

# Compare output
message(STATUS "Comparing output")
message(STATUS "Generated output file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.txt")
message(STATUS "Original output file is ${REF}.txt")

if(EXISTS ${REF}.txt)
	execute_process(COMMAND ${DIFF} ${DST}.txt ${REF}.txt RESULT_VARIABLE HAD_ERROR)
	if(HAD_ERROR)
		message(FATAL_ERROR "Output different")
	endif(HAD_ERROR)
else()
	message(STATUS "No output reference")
endif()

# Compare pcap
message(STATUS "Comparing pcap")
message(STATUS "Generated pcap file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.pcap")
message(STATUS "Original pcap file is ${REF}.pcap")

if(EXISTS ${REF}.pcap)
	execute_process(COMMAND bash ${CTEST_MODULE_DIR}/ComparePcap.sh ${DST}.pcap ${REF}.pcap
		RESULT_VARIABLE HAD_ERROR)
	if(HAD_ERROR)
		message(FATAL_ERROR "Pcap different")
	endif(HAD_ERROR)
else()
	message(STATUS "No pcap reference")
endif()
