# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_definitions(-DLUA_COMPAT_ALL=1)

add_subdirectory(external/lua)

set(HAKA_LUAJIT 0)
set(HAKA_LUA52 1)
