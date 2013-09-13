
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION ${HAKA_INSTALL_PREFIX}/include/haka)

install(DIRECTORY . DESTINATION ${HAKA_INSTALL_PREFIX}/include/haka FILES_MATCHING PATTERN "*.h" PATTERN "*.si")
