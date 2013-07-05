
# Setup rpath for modules
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

# Build main program
add_executable(haka
	main.c
	app.c
	thread.c)

target_link_libraries(haka libhaka)
target_link_libraries(haka ${CMAKE_THREAD_LIBS_INIT})
LUA_LINK(haka)

include_directories(.)

add_subdirectory(lua)
target_link_libraries(haka haka-lua)

install(TARGETS haka RUNTIME DESTINATION bin)

add_subdirectory(conf)
add_subdirectory(test)
