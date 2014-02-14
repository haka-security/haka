# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(TestUnit)
include(TestUnitLua)

get_property(module-regexp GLOBAL PROPERTY module-regexp)
foreach(module IN LISTS module-regexp)
	TEST_UNIT(MODULE libhaka NAME regexp-${module} FILES regexp.c ENV "HAKA_MODULE=${module}")
	target_link_libraries(libhaka-regexp-${module} libhaka)

	TEST_UNIT_LUA(MODULE libhaka NAME regexp-${module}-lua FILES regexp-lua ENV "HAKA_MODULE=${module}")
endforeach(module)
