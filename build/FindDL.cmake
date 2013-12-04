# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Search for dlopen.
# DL_FOUND - system has dynamic linking interface available
# DL_INCLUDE_DIR - where dlfcn.h is located.
# DL_LIBRARIES - libraries needed to use dlopen

include(CheckFunctionExists)
include(FindPackageHandleStandardArgs)

find_path(DL_INCLUDE_DIR NAMES dlfcn.h)
find_library(DL_LIBRARIES NAMES dl)

if(DL_LIBRARIES)
	set(DL_FOUND)
else(DL_LIBRARIES)
	# Check if dlopen if part of the libc.
	check_function_exists(dlopen DL_FOUND)
	set(DL_LIBRARIES "")
endif(DL_LIBRARIES)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(DL
	REQUIRED_VARS DL_LIBRARIES DL_INCLUDE_DIR)
