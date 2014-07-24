# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(LIBGEOIP_INCLUDE_DIR GeoIP.h)
find_library(LIBGEOIP_LIBRARY NAMES GeoIP)

if(LIBGEOIP_INCLUDE_DIR AND LIBGEOIP_LIBRARY)
	set(LIBGEOIP_FOUND)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibGeoIP
	REQUIRED_VARS LIBGEOIP_LIBRARY LIBGEOIP_INCLUDE_DIR)
