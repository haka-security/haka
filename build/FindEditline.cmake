
find_path(EDITLINE_INCLUDE_DIR editline/readline.h)
find_library(EDITLINE_LIBRARY NAMES edit)

if(EDITLINE_INCLUDE_DIR AND READLINE_LIBRARY)
	set(EDITLINE_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(EDITLINE
	REQUIRED_VARS EDITLINE_LIBRARY EDITLINE_INCLUDE_DIR)
