
macro(TEST_MODULE_LOAD name)
	add_test(NAME ${name}-load
	    COMMAND ${CMAKE_COMMAND}
		-DEXE=$<TARGET_FILE:haka>
		-DMODULE=${name}
		-DCMAKE_TEMP_DIR=${CMAKE_SOURCE_DIR}/Testing/Temporary
		-P ${CTEST_MODULE_DIR}/TestModuleLoadRun.cmake)
endmacro(TEST_MODULE_LOAD)

