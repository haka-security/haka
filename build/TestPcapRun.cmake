# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(ENV{LANG} "C")
set(ENV{LC_ALL} "C")
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{DIFF} ${DIFF})
set(ENV{TSHARK} ${TSHARK})
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)
set(ENV{HAKA_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX})
set(ENV{LD_LIBRARY_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/lib)
set(ENV{TZ} Europe/Paris)

set(CMAKE_MODULE_PATH ${CTEST_MODULE_DIR} ${CMAKE_MODULE_PATH})
include(Valgrind)

message("Executing TZ=\"Europe/Paris\" LANG=\"C\" LC_ALL=\"C\" LUA_PATH=\"$ENV{LUA_PATH}\" HAKA_PATH=\"$ENV{HAKA_PATH}\" LD_LIBRARY_PATH=\"$ENV{LD_LIBRARY_PATH}\" ${EXE} -d --no-pass-through ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}")

VALGRIND_FULL(${DST} ${EXE} -d --no-pass-through ${EXE_OPTIONS} -o ${DST}.pcap ${SRC} ${CONF}
	RESULT_VARIABLE HAD_ERROR OUTPUT_FILE ${DST}-tmp.txt)

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

if(EXISTS "${REF}-${REFVER}.txt")
	set(REFTXT "${REF}-${REFVER}.txt")
elseif(EXISTS "${REF}.txt")
	set(REFTXT "${REF}.txt")
endif()

if(EXISTS "${REFTXT}")
	execute_process(COMMAND ${DIFF} -u ${REFTXT} ${DST}.txt RESULT_VARIABLE HAD_ERROR OUTPUT_VARIABLE CONTENT)
	if(HAD_ERROR)
		message("${CONTENT}")
	endif(HAD_ERROR)
	message("Generated output file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.txt")
	message("Original output file is ${REFTXT}")
	if(HAD_ERROR)
		if(NOT "$ENV{HAKA_TEST_FIX}" STREQUAL "")
			execute_process(COMMAND echo cp ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.txt ${REFTXT} OUTPUT_FILE $ENV{HAKA_TEST_FIX})
		endif()
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

if(EXISTS "${REF}-${REFVER}.pcap")
	set(REFPCAP "${REF}-${REFVER}.pcap")
elseif(EXISTS "${REF}.pcap")
	set(REFPCAP "${REF}.pcap")
endif()

if(EXISTS "${REFPCAP}")
	execute_process(COMMAND bash ${CTEST_MODULE_DIR}/ComparePcap.sh ${REFPCAP} ${DST}.pcap
		RESULT_VARIABLE HAD_ERROR OUTPUT_VARIABLE CONTENT)
	if(HAD_ERROR)
		message("${CONTENT}")
	endif(HAD_ERROR)
	message("Generated pcap file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.pcap")
	message("Original pcap file is ${REFPCAP}")
	if(HAD_ERROR)
		if(ENV{HAKA_TEST_FIX})
			execute_process(COMMAND echo cp ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.pcap ${REFPCAP} OUTPUT_FILE $ENV{HAKA_TEST_FIX})
		endif()
		message(FATAL_ERROR "Pcap different")
	endif(HAD_ERROR)
else()
	message("Generated pcap file is ${CMAKE_CURRENT_SOURCE_DIR}/${DST}.pcap")
	message("Original pcap file is ${REF}.pcap")
	message("No pcap reference")
endif()

# Memory leak
CHECK_VALGRIND(${DST})
