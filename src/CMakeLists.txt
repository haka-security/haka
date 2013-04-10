
# Setup rpath for modules
set(CMAKE_INSTALL_RPATH "$ORIGIN/modules" "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

# Build main program
add_executable(haka
	main.c
	app.c)

target_link_libraries(haka libhaka)

include_directories(.)

add_subdirectory(lua)
target_link_libraries(haka haka-lua)

install(TARGETS haka RUNTIME DESTINATION bin)
