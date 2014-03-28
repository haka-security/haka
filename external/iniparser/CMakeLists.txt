# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(libiniparser STATIC
	src/src/dictionary.c
	src/src/iniparser.c)

# The static library is going to be used in a shared library
SET_TARGET_PROPERTIES(libiniparser PROPERTIES COMPILE_FLAGS "-fPIC")

set(LIBINIPARSER_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src/src" CACHE INTERNAL "iniparser include path")
