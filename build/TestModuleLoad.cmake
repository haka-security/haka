
macro(TEST_MODULE_LOAD name)
	add_test(NAME ${name}-load
		COMMAND ${CMAKE_COMMAND}
		-DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
		-DEXE=$<TARGET_FILE:haka>
		-DMODULE=${name}
		-DCMAKE_TEMP_DIR=${CMAKE_BINARY_DIR}/Testing/Temporary
		-P ${CTEST_MODULE_DIR}/TestModuleLoadRun.cmake)
endmacro(TEST_MODULE_LOAD)

