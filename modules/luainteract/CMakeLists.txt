
find_package(Readline)
if(READLINE_FOUND)
	find_package(SWIG REQUIRED)
	include(UseSWIG)
	swig_add_module(luainteract lua SHARED
		interact.i
		session.c
		complete.c
	)

	LUA_LINK(luainteract)
	target_link_libraries(luainteract libhaka)
	target_link_libraries(luainteract ${READLINE_LIBRARY})
	INSTALL_MODULE(luainteract)
endif(READLINE_FOUND)
