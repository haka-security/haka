# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Try to find the Check libraries (Unit test framework for C)

include(FindPackageHandleStandardArgs)

find_path(CHECK_INCLUDE_DIRS NAMES check.h)
find_library(CHECK_LIBRARIES NAMES check)
if(CHECK_LIBRARIES)
	set(CHECK_FOUND)
endif(CHECK_LIBRARIES)

list(APPEND CHECK_LIBRARIES rt)
list(APPEND CHECK_LIBRARIES m)
list(APPEND CHECK_LIBRARIES pthread)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CHECK
	REQUIRED_VARS CHECK_LIBRARIES CHECK_INCLUDE_DIRS)
