
add_library(libiniparser STATIC
	iniparser/src/dictionary.c
	iniparser/src/iniparser.c)

# The static library is going to be used in a shared library
SET_TARGET_PROPERTIES(libiniparser PROPERTIES COMPILE_FLAGS "-fPIC")

set(LIBINIPARSER_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/iniparser/src" PARENT_SCOPE)
