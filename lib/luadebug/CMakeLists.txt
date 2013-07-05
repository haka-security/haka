
find_package(Readline)
if(READLINE_FOUND)
	include_directories(.)
	
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
	target_link_libraries(libluadebug ${READLINE_LIBRARY})
endif(READLINE_FOUND)
