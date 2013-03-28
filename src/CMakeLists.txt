
# Setup rpath for modules
set(CMAKE_INSTALL_RPATH "$ORIGIN/modules")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

# Build main program
add_executable(haka main.c module.c)
target_link_libraries(haka dl)

install(TARGETS haka RUNTIME DESTINATION bin)

