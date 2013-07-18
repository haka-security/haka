
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(libluadebug lua STATIC
	luadebug.i
	interactive.c
	complete.c
	debugger.c
	readline.c
	utils.c
)

LUA_LINK(libluadebug)

find_package(Editline REQUIRED)
include_directories(. ${EDITLINE_INCLUDE_DIR})
target_link_libraries(libluadebug ${EDITLINE_LIBRARY})

# The static library is going to be used in a shared library
SET_TARGET_PROPERTIES(libluadebug PROPERTIES COMPILE_FLAGS "-fPIC")
