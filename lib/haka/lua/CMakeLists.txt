# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(libhaka-lua lua STATIC
	object.i
	ref.c
	state.c
	)

# Lua dependency
LUA_LINK(libhaka-lua)

# The static library is going to be used in a shared library
SET_TARGET_PROPERTIES(libhaka-lua PROPERTIES COMPILE_FLAGS "-fPIC")
