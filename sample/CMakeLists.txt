# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

install(FILES empty.lua DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/sample)
install(DIRECTORY filter gettingstarted hellopacket ruleset sqli stats smtp_dissector DESTINATION ${HAKA_INSTALL_PREFIX}/share/haka/sample)

add_subdirectory(test)
