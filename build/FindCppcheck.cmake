# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

FIND_PROGRAM(CPPCHECK_EXECUTABLE
	NAMES cppcheck DOC "Tool for static C/C++ code analysis")

IF(CPPCHECK_EXECUTABLE)
	EXECUTE_PROCESS(COMMAND ${CPPCHECK_EXECUTABLE} "--version"
		OUTPUT_VARIABLE _cppcheck_version ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	IF(_cppcheck_version MATCHES "^Cppcheck [0-9].*")
		STRING(REPLACE "Cppcheck " "" CPPCHECK_VERSION "${_cppcheck_version}")
	ENDIF()

	UNSET(_cppcheck_version)
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cppcheck REQUIRED_VARS CPPCHECK_EXECUTABLE
	VERSION_VAR CPPCHECK_VERSION)
