# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(TestPcap)
include(TestUnit)
include(TestUnitLua)

TEST_UNIT(MODULE libhaka NAME vbuffer FILES vbuffer.c LIBS libhaka)

TEST_UNIT(MODULE libhaka NAME vbuffer-stream FILES vbuffer_stream.c LIBS libhaka)

TEST_UNIT(MODULE libhaka NAME bitfield FILES bitfield.c)
target_link_libraries(libhaka-bitfield libhaka)

TEST_UNIT_LUA(MODULE libhaka NAME vbuffer FILES vbuffer.lua)
TEST_UNIT_LUA(MODULE libhaka NAME vbuffer_stream_blocking FILES vbuffer_stream_blocking.lua)

TEST_UNIT_LUA(MODULE libhaka NAME grammar-union FILES grammar-union.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-try FILES grammar-try.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-token FILES grammar-token.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-inheritance FILES grammar-inheritance.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-apply FILES grammar-apply.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-sequence FILES grammar-sequence.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-empty FILES grammar-empty.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-bytes FILES grammar-bytes.lua)
TEST_UNIT_LUA(MODULE libhaka NAME state-machine FILES state-machine.lua)
TEST_UNIT_LUA(MODULE libhaka NAME dissector-register FILES dissector-register.lua)

get_property(module-regexp GLOBAL PROPERTY module-regexp)
foreach(module IN LISTS module-regexp)
	TEST_UNIT(MODULE libhaka NAME regexp-${module} FILES regexp.c ENV "HAKA_MODULE=${module}" LIBS libhaka)

	TEST_UNIT_LUA(MODULE libhaka NAME regexp-${module} FILES regexp-lua ENV "HAKA_MODULE=${module}")
endforeach(module)

TEST_PCAP(libhaka basic)
TEST_PCAP(libhaka nodrop)
