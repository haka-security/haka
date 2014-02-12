# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This test will run a Check unit test program build from the sources
# files given as argument.

find_package(Check REQUIRED)

macro(TEST_UNIT module name files)
	add_executable(${module}-${name} ${files})
	target_link_libraries(${module}-${name} ${CHECK_LIBRARIES})
	set_property(TARGET ${module}-${name} APPEND PROPERTY INCLUDE_DIRECTORIES ${CHECK_INCLUDE_DIRS})
	set_property(TARGET ${module}-${name} APPEND PROPERTY LINK_FLAGS -Wl,--exclude-libs,ALL)
	add_test(${module}-${name} ${CMAKE_CURRENT_BINARY_DIR}/${module}-${name})
	set_tests_properties(${module}-${name} PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${LUA_LIBRARY_DIR}")
	set_tests_properties(${module}-${name} PROPERTIES ENVIRONMENT "HAKA_PATH=${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}")
endmacro(TEST_UNIT)
