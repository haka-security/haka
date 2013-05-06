#
# This test will run a Check unit test program build from the sources
# files given as argument.
#

macro(TEST_UNIT name files)
	add_executable(${name} ${files})
	target_link_libraries(${name} ${CHECK_LIBRARIES})
	add_test(${name} ${CTEST_MODULE_DIR}/TestUnit.sh ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_CURRENT_BINARY_DIR}/${name})
endmacro(TEST_UNIT)
