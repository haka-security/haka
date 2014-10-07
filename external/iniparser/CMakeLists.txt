# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

unset(INIPARSER_LIBRARY CACHE)
unset(INIPARSER_INCLUDE_DIR CACHE)

find_package(Iniparser)

if(NOT ${INIPARSER_FOUND})
	message(STATUS "Building static iniparser library from externals")

	add_library(iniparser STATIC
		src/src/dictionary.c
		src/src/iniparser.c)

	# The static library is going to be used in a shared library
	SET_TARGET_PROPERTIES(iniparser PROPERTIES COMPILE_FLAGS "-fPIC")

	set(INIPARSER_LIBRARY iniparser CACHE INTERNAL "iniparser libaray")
	set(INIPARSER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/src" CACHE INTERNAL "iniparser include dir")
endif()
