# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(LIBUUID_INCLUDE_DIR uuid/uuid.h)
find_library(LIBUUID_LIBRARY NAMES uuid)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibUuid
	REQUIRED_VARS LIBUUID_LIBRARY LIBUUID_INCLUDE_DIR)
