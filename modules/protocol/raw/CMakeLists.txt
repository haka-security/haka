# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

ADD_MODULE(NAME raw TYPE protocol FILES main.c raw.c LUA lua/raw.lua SWIG raw.i)

include_directories(.)

INSTALL_MODULE(raw protocol)

# Tests
add_subdirectory(test)
