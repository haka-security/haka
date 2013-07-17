
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{DIFF} ${DIFF})
set(ENV{TSHARK} ${TSHARK})
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)

message("Executing LUA_PATH=\"${PROJECT_SOURCE_DIR}/src/lua/?.lua\" ${EXE} -d ${EXE_OPTIONS} ${CTEST_MODULE_BINARY_DIR}/TestPcap.lua ${CONF} ${SRC} ${DST}.pcap")

if(EXE_OPTIONS)
	string(REPLACE " " ";" EXE_OPTIONS ${EXE_OPTIONS})
endif()

if(VALGRIND)
	execute_process(COMMAND ${VALGRIND} --log-file=${DST}-valgrind.txt ${EXE} -d ${EXE_OPTIONS} ${CTEST_MODULE_BINARY_DIR}/TestPcap.lua ${CONF} ${SRC} ${DST}.pcap
		RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)
else()
	execute_process(COMMAND ${EXE} -d ${EXE_OPTIONS} ${CTEST_MODULE_BINARY_DIR}/TestPcap.lua ${CONF} ${SRC} ${DST}.pcap
		RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)
endif()

execute_process(COMMAND cat ${DST}-tmp.txt OUTPUT_VARIABLE CONTENT)
message("${CONTENT}")

if(HAD_ERROR)
	message(FATAL_ERROR "Haka script failed")
endif(HAD_ERROR)

# Filter out some initialization and exit messages
execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CompareOutput.awk ${DST}-tmp.txt OUTPUT_FILE ${DST}.txt)

# Compare output
message("")
message("-- Comparing output")

if(EXISTS ${REF}.txt)
	execute_process(COMMAND ${DIFF} ${DST}.txt ${REF}.txt RESULT_VARIABLE HAD_ERROR OUTPUT_VARIABLE CONTENT)
	message("${CONTENT}")
	message("Generated output file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.txt")
	message("Original output file is ${REF}.txt")
	if(HAD_ERROR)
		message(FATAL_ERROR "Output different")
	endif(HAD_ERROR)
else()
	message("Generated output file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.txt")
	message("Original output file is ${REF}.txt")
	message("No output reference")
endif()

# Compare pcap
message("")
message("-- Comparing pcap")

if(EXISTS ${REF}.pcap)
	execute_process(COMMAND bash ${CTEST_MODULE_DIR}/ComparePcap.sh ${DST}.pcap ${REF}.pcap
		RESULT_VARIABLE HAD_ERROR OUTPUT_VARIABLE CONTENT)
	message("${CONTENT}")
	message("Generated pcap file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.pcap")
	message("Original pcap file is ${REF}.pcap")
	if(HAD_ERROR)
		message(FATAL_ERROR "Pcap different")
	endif(HAD_ERROR)
else()
	message("Generated pcap file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.pcap")
	message("Original pcap file is ${REF}.pcap")
	message("No pcap reference")
endif()

# Memory leak
if(VALGRIND)
	message("")
	message("-- Memory leak check")
	execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CheckValgrind.awk ${DST}-valgrind.txt OUTPUT_VARIABLE VALGRIND_OUT)
	list(GET VALGRIND_OUT 0 VALGRIND_LEAK)
	list(GET VALGRIND_OUT 1 VALGRIND_REACHABLE)

	if (VALGRIND_LEAK GREATER 0)
		message(FATAL_ERROR "Memory leak detected: ${VALGRIND_LEAK} lost bytes")
	endif()

	if (VALGRIND_REACHABLE GREATER 32)
		message(FATAL_ERROR "Memory leak detected: ${VALGRIND_REACHABLE} reachable bytes")
	endif()
	
	message("No leak detected")
endif()
