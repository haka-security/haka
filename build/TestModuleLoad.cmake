
macro(TEST_MODULE_LOAD name type)
	get_target_property(MODULE ${name} OUTPUT_NAME)
	if(MODULE STREQUAL "MODULE-NOTFOUND")
		set(MODULE "${name}")
	endif()

	set(MODULE "${type}/${MODULE}")

	configure_file(${CTEST_MODULE_DIR}/TestModuleLoad.lua.in TestModuleLoad-${name}.lua)

	add_test(NAME ${name}-load
		COMMAND ${CMAKE_COMMAND}
		-DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
		-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DEXE=${TEST_RUNDIR}/bin/haka
		-DTEST_RUNDIR=${TEST_RUNDIR}
		-DLUASCRIPT=TestModuleLoad-${name}.lua
		-P ${CTEST_MODULE_DIR}/TestModuleLoadRun.cmake)
endmacro(TEST_MODULE_LOAD)
