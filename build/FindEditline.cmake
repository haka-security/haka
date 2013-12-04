# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(EDITLINE_INCLUDE_DIR editline/readline.h)
find_library(EDITLINE_LIBRARY NAMES edit)

if(EDITLINE_INCLUDE_DIR AND EDITLINE_LIBRARY)
	set(EDITLINE_FOUND)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(EDITLINE
	REQUIRED_VARS EDITLINE_LIBRARY EDITLINE_INCLUDE_DIR)
