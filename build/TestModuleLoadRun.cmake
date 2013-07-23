set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)

message(STATUS "All outputs and commands for this test are located in ${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Executing LUA_PATH=\"${PROJECT_SOURCE_DIR}/src/lua/?.lua\" ${EXE} -d ${LUASCRIPT}")

execute_process(COMMAND ${EXE} -d ${LUASCRIPT} RESULT_VARIABLE HAD_ERROR)
if(HAD_ERROR)
	message(FATAL_ERROR "Test failed")
endif()
