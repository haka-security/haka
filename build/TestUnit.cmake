#
# This test will run a Check unit test program build from the sources
# files given as argument.
#

macro(TEST_UNIT module name files)
	add_executable(${module}-${name} ${files})
	target_link_libraries(${module}-${name} ${CHECK_LIBRARIES})
	add_test(${module}-${name} ${CTEST_MODULE_DIR}/TestUnit.sh
		"${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/share/haka/modules"
		${CMAKE_CURRENT_BINARY_DIR}/${module}-${name})
endmacro(TEST_UNIT)
