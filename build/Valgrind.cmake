# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_program(VALGRIND_COMMAND valgrind)

find_program(GAWK_COMMAND gawk)
if (NOT GAWK_COMMAND)
	message(FATAL_ERROR "Cannot find gawk command")
endif(NOT GAWK_COMMAND)

if(VALGRIND_COMMAND AND NOT "$ENV{VALGRIND}" STREQUAL "0")
	set(DO_VALGRIND 1)
endif()

macro(RUN_VALGRIND name reachable)
	if (DO_VALGRIND)
		execute_process(COMMAND ${VALGRIND_COMMAND} --leak-check=full --gen-suppressions=all
			--suppressions=${CTEST_MODULE_DIR}/Valgrind.sup
			--suppressions=${CTEST_MODULE_DIR}/Valgrind-check.sup
			--show-reachable=${reachable} --log-file=${name}-valgrind.txt ${ARGN})
	else()
		execute_process(COMMAND ${ARGN})
	endif()
endmacro(RUN_VALGRIND)

macro(VALGRIND name)
	RUN_VALGRIND(${name} no ${ARGN})
	set(DO_VALGRIND_FULL 0)
endmacro(VALGRIND)

macro(VALGRIND_FULL name)
	RUN_VALGRIND(${name} yes ${ARGN})
	set(DO_VALGRIND_FULL 1)
endmacro(VALGRIND_FULL)

macro(CHECK_VALGRIND name)
	if (DO_VALGRIND)
		execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CheckValgrind.awk ${name}-valgrind.txt OUTPUT_VARIABLE VALGRIND_OUT)
		list(GET VALGRIND_OUT 0 VALGRIND_ERROR)
		list(GET VALGRIND_OUT 1 VALGRIND_LEAK)
		list(GET VALGRIND_OUT 2 VALGRIND_REACHABLE)

		message("")
		message("-- Memory error check")
		if(VALGRIND_ERROR GREATER 0 OR VALGRIND_REACHABLE GREATER 0)
			execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/FilterValgrind.awk "${CMAKE_CURRENT_SOURCE_DIR}/${name}-valgrind.txt"
				OUTPUT_VARIABLE VALGRIND_ERRORS)
			message("${VALGRIND_ERRORS}")
		endif()
		message("Valgrind log is at ${CMAKE_CURRENT_SOURCE_DIR}/${name}-valgrind.txt")

		if (DO_VALGRIND_FULL)
			if(VALGRIND_ERROR GREATER 0 OR VALGRIND_REACHABLE GREATER 0)
				message(FATAL_ERROR "Memory error detected: ${VALGRIND_ERROR} error found, ${VALGRIND_REACHABLE} bytes still reachable memory found")
			endif()
		else()
			if(VALGRIND_ERROR GREATER 0)
				message(FATAL_ERROR "Memory error detected: ${VALGRIND_ERROR} error found")
			endif()
		endif()

		message("No error detected")
	endif()
endmacro(CHECK_VALGRIND)
