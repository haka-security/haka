#
# This test will run a Check unit test program build from the sources
# files given as argument.
#

macro(TEST_UNIT module name files)
	add_executable(${module}-${name} ${files})
	target_link_libraries(${module}-${name} ${CHECK_LIBRARIES})
	target_link_libraries(${module}-${name} ${CMAKE_THREAD_LIBS_INIT})
	add_test(${module}-${name} ${CMAKE_CURRENT_BINARY_DIR}/${module}-${name})
endmacro(TEST_UNIT)
