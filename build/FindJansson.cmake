# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(JANSSON_INCLUDE_DIR jansson.h)
find_library(JANSSON_LIBRARY NAMES jansson)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Jansson
	REQUIRED_VARS JANSSON_LIBRARY JANSSON_INCLUDE_DIR)
