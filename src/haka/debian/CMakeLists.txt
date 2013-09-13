
configure_file(init.in ${CMAKE_CURRENT_BINARY_DIR}/init)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/init DESTINATION etc/init.d RENAME haka)

configure_file(preinst.in ${CMAKE_CURRENT_BINARY_DIR}/preinst)
configure_file(postinst.in ${CMAKE_CURRENT_BINARY_DIR}/postinst)
configure_file(prerm.in ${CMAKE_CURRENT_BINARY_DIR}/prerm)
configure_file(postrm.in ${CMAKE_CURRENT_BINARY_DIR}/postrm)
configure_file(conffiles.in ${CMAKE_CURRENT_BINARY_DIR}/conffiles)
