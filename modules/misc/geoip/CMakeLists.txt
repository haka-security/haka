# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(LibGeoIP)

if(LIBGEOIP_FOUND)
	INCLUDE_MODULE(ipv4 protocol)

	swig_process(geoipswig lua geoip.i)

	add_library(geoip MODULE
		main.c
		geoip.c
		${SWIG_geoipswig_FILES}
	)

	include_directories(${LIBGEOIP_INCLUDE_DIR})
	target_link_libraries(geoip ${LIBGEOIP_LIBRARY})

	SWIG_FIX_ENTRYPOINT(geoip misc)

	INCLUDE_MODULE(geoip ipv4 protocol)

	INSTALL_MODULE(geoip misc)

	add_subdirectory(test)
else()
    message(STATUS "Not building module geoip (missing libraries)")
endif()
