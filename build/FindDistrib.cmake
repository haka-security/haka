# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

macro(MATCH_FILE file match var)
	if(EXISTS ${file})
		file(READ ${file} content)
		if("${content}" MATCHES "${match}")
			set(${var} TRUE)
		endif()
	endif()
endmacro()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	MATCH_FILE(/etc/os-release "ID=debian" debian)
	MATCH_FILE(/etc/os-release "ID_LIKE=debian" debian)
	MATCH_FILE(/etc/redhat-release "" redhat)

	if(debian)
		set(DISTRIB_NAME "Debian")
		set(DISTRIB_PACKAGE "DEB")
	elseif(redhat)
		set(DISTRIB_NAME "Redhat")
		set(DISTRIB_PACKAGE "RPM")
	else()
		set(DISTRIB_NAME "Linux")
		set(DISTRIB_PACKAGE "TGZ")
	endif()
endif()

if(NOT DISTRIB_NAME)
	message(FATAL_ERROR "unsupported distribution")
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Distrib
	REQUIRED_VARS DISTRIB_NAME DISTRIB_PACKAGE)
