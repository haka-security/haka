
# Setup rpath
set(CMAKE_INSTALL_RPATH "${CMAKE_HAKA_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

set(HAKA_PID_FILE "${CMAKE_INSTALL_PREFIX}/var/run/haka.pid")
set(HAKA_CTL_SOCKET_FILE "${CMAKE_INSTALL_PREFIX}/var/run/haka-ctl.sock")

add_subdirectory(common)
add_subdirectory(haka)
add_subdirectory(hakactl)
add_subdirectory(test)
