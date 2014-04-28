# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

macro(find_blockdiag_gen name)
	string(TOUPPER ${name} upper_name)
	string(TOLOWER ${name} lower_name)

	FIND_PROGRAM(${upper_name}_EXECUTABLE
		NAMES ${lower_name} DOC "Tool for rendering ${lower_name} diagram")

	IF(SEQDIAG_EXECUTABLE)
		EXECUTE_PROCESS(COMMAND ${${upper_name}_EXECUTABLE} "--version"
			OUTPUT_VARIABLE _version ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE)

		IF(_version MATCHES "^${lower_name} .*")
			STRING(REGEX REPLACE "^${lower_name} ([^ ]+)" "\\1" ${upper_name}_VERSION "${_version}")
		ENDIF()

		UNSET(_version)
	ENDIF()

	FIND_PACKAGE_HANDLE_STANDARD_ARGS(${name} REQUIRED_VARS ${upper_name}_EXECUTABLE
		VERSION_VAR ${upper_name}_VERSION)
endmacro()
