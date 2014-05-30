# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(TestUnit)
include(TestUnitLua)

TEST_UNIT(MODULE libhaka NAME vbuffer FILES vbuffer.c)
target_link_libraries(libhaka-vbuffer libhaka)

TEST_UNIT(MODULE libhaka NAME vbuffer-stream FILES vbuffer_stream.c)
target_link_libraries(libhaka-vbuffer-stream libhaka)

TEST_UNIT_LUA(MODULE libhaka NAME vbuffer FILES vbuffer.lua)
TEST_UNIT_LUA(MODULE libhaka NAME vbuffer_stream_blocking FILES vbuffer_stream_blocking.lua)

TEST_UNIT_LUA(MODULE libhaka NAME grammar-union FILES grammar-union.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-try FILES grammar-try.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-token FILES grammar-token.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-inheritance FILES grammar-inheritance.lua)
TEST_UNIT_LUA(MODULE libhaka NAME grammar-apply FILES grammar-apply.lua)

get_property(module-regexp GLOBAL PROPERTY module-regexp)
foreach(module IN LISTS module-regexp)
	TEST_UNIT(MODULE libhaka NAME regexp-${module} FILES regexp.c ENV "HAKA_MODULE=${module}")
	target_link_libraries(libhaka-regexp-${module} libhaka)

	TEST_UNIT_LUA(MODULE libhaka NAME regexp-${module} FILES regexp-lua ENV "HAKA_MODULE=${module}")
endforeach(module)

