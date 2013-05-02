
# Setup rpath for modules
set(CMAKE_INSTALL_RPATH "$ORIGIN/modules" "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

# Lua dependency
find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# Build main program
add_executable(haka
	main.c
	app.c
	thread.c)

target_link_libraries(haka libhaka)
target_link_libraries(haka ${LUA_LIBRARY})
target_link_libraries(haka ${CMAKE_THREAD_LIBS_INIT})

include_directories(.)

add_subdirectory(lua)
target_link_libraries(haka haka-lua)

install(TARGETS haka RUNTIME DESTINATION bin)
