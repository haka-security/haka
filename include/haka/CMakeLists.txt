# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION ${HAKA_INSTALL_PREFIX}/include/haka)

configure_file(lua/config.si.in ${CMAKE_CURRENT_BINARY_DIR}/lua/config.si)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lua/config.si DESTINATION ${HAKA_INSTALL_PREFIX}/include/haka/lua)

install(DIRECTORY . DESTINATION ${HAKA_INSTALL_PREFIX}/include/haka FILES_MATCHING PATTERN "*.h" PATTERN "*.si")
