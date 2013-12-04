# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(libluadebug lua STATIC
	luadebug.i
	interactive.c
	complete.c
	debugger.c
	utils.c
	user.c
	user_readline.c
	user_remote.c
)

LUA_LINK(libluadebug)

find_package(Editline REQUIRED)
include_directories(. ${EDITLINE_INCLUDE_DIR})
target_link_libraries(libluadebug ${EDITLINE_LIBRARY})
target_link_libraries(libluadebug libhaka)

# The static library is going to be used in a shared library
set_target_properties(libluadebug PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(libluadebug PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)
