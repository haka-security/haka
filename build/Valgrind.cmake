
find_program(VALGRIND_COMMAND valgrind)

find_program(GAWK_COMMAND gawk)
if (NOT GAWK_COMMAND)
	message(FATAL_ERROR "Cannot find gawk command")
endif(NOT GAWK_COMMAND)

if(VALGRIND_COMMAND AND NOT "$ENV{QUICK}" STREQUAL "yes")
	set(DO_VALGRIND 1)
endif()

macro(VALGRIND name)
	if (DO_VALGRIND)
		execute_process(COMMAND ${VALGRIND_COMMAND} --leak-check=full --gen-suppressions=all
			--suppressions=${CTEST_MODULE_DIR}/Valgrind.sup --show-reachable=yes
			--log-file=${name}-valgrind.txt ${ARGN})
	else()
		execute_process(COMMAND ${ARGN})
	endif()
endmacro(VALGRIND)

macro(CHECK_VALGRIND name)
	if (DO_VALGRIND)
		execute_process(COMMAND gawk -f ${CTEST_MODULE_DIR}/CheckValgrind.awk ${name}-valgrind.txt OUTPUT_VARIABLE VALGRIND_OUT)
		list(GET VALGRIND_OUT 0 VALGRIND_ERROR)
		list(GET VALGRIND_OUT 1 VALGRIND_LEAK)
		list(GET VALGRIND_OUT 2 VALGRIND_REACHABLE)

		message("")
		message("-- Memory error check")
		message("Valgrind log is at ${CMAKE_CURRENT_SOURCE_DIR}/${name}-valgrind.txt")
		if(VALGRIND_ERROR GREATER 0 OR VALGRIND_REACHABLE GREATER 0)
			message(FATAL_ERROR "Memory error detected: ${VALGRIND_ERROR} error found, ${VALGRIND_REACHABLE} bytes still reachable memory found")
		endif()
		message("No error detected")
	endif()
endmacro(CHECK_VALGRIND)
