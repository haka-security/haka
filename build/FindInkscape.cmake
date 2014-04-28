# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

FIND_PROGRAM(INKSCAPE_EXECUTABLE
	NAMES inkscape DOC "Tool for rendering svg pictures")

IF(INKSCAPE_EXECUTABLE)
	EXECUTE_PROCESS(COMMAND ${INKSCAPE_EXECUTABLE} "--version"
		OUTPUT_VARIABLE _inkscape_version ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	IF(_inkscape_version MATCHES "^Inkscape .*")
		STRING(REGEX REPLACE "^Inkscape ([^ ]+) .*" "\\1" INKSCAPE_VERSION "${_inkscape_version}")
	ENDIF()

	UNSET(_inkscape_version)
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Inkscape REQUIRED_VARS INKSCAPE_EXECUTABLE
	VERSION_VAR INKSCAPE_VERSION)
