
# Setup rpath
set(CMAKE_INSTALL_RPATH "${HAKA_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

include_directories(.)

# Lua
add_subdirectory(lua)

# Build common sources
add_library(haka-common STATIC
	app.c
	thread.c)
	
add_dependencies(haka-common ${LUA_DEPENDS})

# Build haka program
add_executable(haka
	haka.c)

target_link_libraries(haka libhaka)
target_link_libraries(haka haka-common)
target_link_libraries(haka ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(haka haka-liblua)
LUA_LINK(haka)

install(TARGETS haka RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/bin)

install(FILES haka.conf DESTINATION ${HAKA_INSTALL_PREFIX}/etc/haka)

configure_file(init.debian.in ${CMAKE_CURRENT_BINARY_DIR}/init.debian)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/init.debian DESTINATION etc/init.d RENAME haka)

# Build hakapcap program
add_executable(hakapcap
	hakapcap.c)

target_link_libraries(hakapcap libhaka)
target_link_libraries(hakapcap haka-common)
target_link_libraries(hakapcap ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(hakapcap haka-liblua)
LUA_LINK(hakapcap)

install(TARGETS hakapcap RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/bin)
