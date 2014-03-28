# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)
include(TestUnitLua)

TEST_PCAP(doc doc-alert CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/../ref/alert-test.lua)
TEST_UNIT_LUA(MODULE core NAME doc-regexp FILES ../ref/regexp.lua)
