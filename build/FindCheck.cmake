# Try to find the Check libraries (Unit test framework for C)

include(FindPackageHandleStandardArgs)

find_path(CHECK_INCLUDE_DIRS NAMES check.h)
find_library(CHECK_LIBRARIES NAMES check_pic)
if(CHECK_LIBRARIES)
	set(CHECK_FOUND)
endif(CHECK_LIBRARIES)

list(APPEND CHECK_LIBRARIES rt)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CHECK
	REQUIRED_VARS CHECK_LIBRARIES CHECK_INCLUDE_DIRS)
