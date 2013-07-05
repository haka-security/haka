
find_package(Readline)
if(READLINE_FOUND)
	include_directories(.)
	
	find_package(SWIG REQUIRED)
	include(UseSWIG)
	swig_add_module(luadebug lua STATIC
		interact.i
		session.c
		complete.c
		debugger.c
		readline.c
		utils.c
	)

	LUA_LINK(luadebug)
	target_link_libraries(luadebug libhaka)
	target_link_libraries(luadebug ${READLINE_LIBRARY})
endif(READLINE_FOUND)
