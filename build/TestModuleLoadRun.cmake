set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)

configure_file(${CMAKE_CURRENT_LIST_DIR}/TestModuleLoad.lua.in ${CMAKE_TEMP_DIR}/TestModuleLoad.lua)

message(STATUS "All outputs and commands for this test are located in ${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Executing LUA_PATH=\"${PROJECT_SOURCE_DIR}/src/lua/?.lua\" ${EXE} -d ${CMAKE_TEMP_DIR}/TestModuleLoad.lua")

execute_process(COMMAND ${EXE} -d ${CMAKE_TEMP_DIR}/TestModuleLoad.lua
	RESULT_VARIABLE HAD_ERROR)
if(HAD_ERROR)
	message(FATAL_ERROR "Test failed")
endif()
