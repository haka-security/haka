
include(FindPackageHandleStandardArgs)

FIND_PROGRAM(CPPCHECK_EXECUTABLE
	NAMES cppcheck
	DOC "Tool for static C/C++ code analysis"
)

IF(CPPCHECK_EXECUTABLE)
	EXECUTE_PROCESS(COMMAND ${CPPCHECK_EXECUTABLE} "--version"
		OUTPUT_VARIABLE cppcheck_version
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	if (cppcheck_version MATCHES "^Cppcheck [0-9].*")
		string(REPLACE "Cppcheck " "" CPPCHECK_VERSION "${cppcheck_version}")
	endif()
	unset(cppcheck_version)
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cppcheck REQUIRED_VARS CPPCHECK_EXECUTABLE VERSION_VAR CPPCHECK_VERSION)
