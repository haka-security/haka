
macro(TEST_MODULE_LOAD name)
	set(MODULE ${name})
	configure_file(${CTEST_MODULE_DIR}/TestModuleLoad.lua.in TestModuleLoad-${name}.lua)

	add_test(NAME ${name}-load
		COMMAND ${CMAKE_COMMAND}
		-DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=$<TARGET_FILE:haka>
		-DCMAKE_TEMP_DIR=${CMAKE_BINARY_DIR}/Testing/Temporary
		-DLUASCRIPT=TestModuleLoad-${name}.lua
		-P ${CTEST_MODULE_DIR}/TestModuleLoadRun.cmake)
endmacro(TEST_MODULE_LOAD)

