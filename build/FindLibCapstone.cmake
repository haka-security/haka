# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(LIBCAPSTONE_INCLUDE_DIR capstone.h PATH_SUFFIXES capstone)
find_library(LIBCAPSTONE_LIBRARY NAMES capstone)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibCapstone
	REQUIRED_VARS LIBCAPSTONE_LIBRARY LIBCAPSTONE_INCLUDE_DIR)

