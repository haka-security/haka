
configure_file(${CMAKE_CURRENT_LIST_DIR}/TestModuleLoad.lua.cmake ${CMAKE_TEMP_DIR}/TestModuleLoad.lua)

execute_process(COMMAND bash ${CTEST_MODULE_DIR}/PrintEnviron.sh)

execute_process(COMMAND ${EXE} ${CMAKE_TEMP_DIR}/TestModuleLoad.lua
    RESULT_VARIABLE HAD_ERROR)
if(HAD_ERROR)
    message(FATAL_ERROR "Test failed")
endif()

