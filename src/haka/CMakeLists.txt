
set(HAKA_PID_FILE "${CMAKE_INSTALL_PREFIX}/var/run/haka.pid")
set(HAKA_CTL_SOCKET_FILE "${CMAKE_INSTALL_PREFIX}/var/run/haka-ctl.sock")

# Setup rpath
set(CMAKE_INSTALL_RPATH "${CMAKE_HAKA_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Lua
add_subdirectory(lua)

# Build common sources
add_library(haka-common STATIC
	app.c
	thread.c)

LUA_DEPENDS(haka-common)

# Build haka program
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_executable(haka
	haka.c
	ctl.c)

target_link_libraries(haka libhaka)
target_link_libraries(haka haka-common)
target_link_libraries(haka ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(haka haka-liblua)
LUA_LINK(haka)

install(TARGETS haka RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/bin)

configure_file(haka.conf.in ${CMAKE_CURRENT_BINARY_DIR}/haka.conf)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/haka.conf DESTINATION ${HAKA_INSTALL_PREFIX}/etc/haka)

# Build hakapcap program
add_executable(hakapcap
	hakapcap.c)

target_link_libraries(hakapcap libhaka)
target_link_libraries(hakapcap haka-common)
target_link_libraries(hakapcap ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(hakapcap haka-liblua)
LUA_LINK(hakapcap)

install(TARGETS hakapcap RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/bin)

add_subdirectory(debian)
