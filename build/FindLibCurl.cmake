# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(LIBCURL_INCLUDE_DIR curl/curl.h)
find_library(LIBCURL_LIBRARY NAMES curl)

if(LIBCURL_INCLUDE_DIR AND LIBCURL_LIBRARY)
	set(LIBCURL_FOUND)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibCurl
	REQUIRED_VARS LIBCURL_LIBRARY LIBCURL_INCLUDE_DIR)
