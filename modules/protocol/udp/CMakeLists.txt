# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

lua_compile(NAME udp FILES udp.lua)
lua_install(TARGET udp DESTINATION ${MODULE_INSTALL_PATH}/protocol)

lua_compile(NAME udp-connection FILES udp-connection.lua)
lua_install(TARGET udp-connection DESTINATION ${MODULE_INSTALL_PATH}/protocol)

# Tests
add_subdirectory(test)
