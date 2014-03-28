# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)
include(TestUnitLua)

TEST_PCAP(core rule-error)
TEST_PCAP(core onexit)
TEST_PCAP(core timestamp)
TEST_PCAP(core state_machine)
TEST_PCAP(core payload)
TEST_PCAP(core alert)
TEST_PCAP(core stream-rule)
TEST_PCAP(core help OPTIONS --help)
TEST_UNIT_LUA(MODULE core NAME doc-regexp FILES ../../doc/ref/regexp.lua)
TEST_PCAP(core token-grammar)
TEST_PCAP(core token-grammar-coroutine)
TEST_PCAP(core grammar-array-setter)
