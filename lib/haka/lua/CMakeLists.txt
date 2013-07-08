
# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(libhaka-lua lua STATIC
	packet.i
	object.i
	ref.c
	state.c
	)

# Lua dependency
LUA_LINK(libhaka-lua)

target_link_libraries(libhaka-lua libluadebug)

# The static library is going to be used in a shared library
SET_TARGET_PROPERTIES(libhaka-lua PROPERTIES COMPILE_FLAGS "-fPIC")
