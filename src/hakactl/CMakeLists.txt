
include_directories(.)
include_directories(../common)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Build hakactl program
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_executable(hakactl
	hakactl.c
	commands.c)

target_link_libraries(hakactl libhaka)
target_link_libraries(hakactl haka-common-ctl)
target_link_libraries(hakactl libluadebug)
target_link_libraries(hakactl ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS hakactl RUNTIME DESTINATION ${HAKA_INSTALL_PREFIX}/bin)
