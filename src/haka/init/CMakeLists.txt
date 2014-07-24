# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(DISTRIB_NAME STREQUAL "Debian")

	configure_file(init-debian.in ${CMAKE_CURRENT_BINARY_DIR}/init)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/init DESTINATION etc/init.d RENAME haka)

	configure_file(conffiles.in ${CMAKE_CURRENT_BINARY_DIR}/conffiles)
	configure_file(preinst-debian.in ${CMAKE_CURRENT_BINARY_DIR}/preinst)
	configure_file(postinst-debian.in ${CMAKE_CURRENT_BINARY_DIR}/postinst)
	configure_file(prerm-debian.in ${CMAKE_CURRENT_BINARY_DIR}/prerm)
	configure_file(postrm-debian.in ${CMAKE_CURRENT_BINARY_DIR}/postrm)

elseif(DISTRIB_NAME STREQUAL "Redhat")

	configure_file(init-redhat.in ${CMAKE_CURRENT_BINARY_DIR}/init)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/init DESTINATION etc/init.d RENAME haka)

	configure_file(preinst-redhat.in ${CMAKE_CURRENT_BINARY_DIR}/preinst)
	configure_file(postinst-redhat.in ${CMAKE_CURRENT_BINARY_DIR}/postinst)
	configure_file(prerm-redhat.in ${CMAKE_CURRENT_BINARY_DIR}/prerm)
	configure_file(postrm-redhat.in ${CMAKE_CURRENT_BINARY_DIR}/postrm)

endif()
