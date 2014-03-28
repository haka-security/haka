# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Setup rpath
set(CMAKE_INSTALL_RPATH "${CMAKE_HAKA_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

set(HAKA_PID_FILE "${CMAKE_INSTALL_PREFIX}/var/run/haka.pid")
set(HAKA_CTL_SOCKET_FILE "${CMAKE_INSTALL_PREFIX}/var/run/haka-ctl.sock")

add_subdirectory(haka)
add_subdirectory(hakactl)
