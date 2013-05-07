
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX})

if(USE_LUAJIT)
	# LuaJit
	add_custom_target(luajit
		COMMAND rsync -rt ${CMAKE_CURRENT_SOURCE_DIR}/luajit ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/luajit PREFIX=${INSTALL_DIR} XCFLAGS=-DLUAJIT_ENABLE_LUA52COMPAT
		COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/luajit LDCONFIG='/sbin/ldconfig -n' PREFIX=${INSTALL_DIR} XCFLAGS=-DLUAJIT_ENABLE_LUA52COMPAT INSTALL_X='install -m 0755 -p' INSTALL_F='install -m 0644 -p' install
	)

	set(LUA_INCLUDE_DIR ${INSTALL_DIR}/include/luajit-2.0 PARENT_SCOPE)
	set(LUA_LIBRARY ${INSTALL_DIR}/lib/libluajit-5.1.so PARENT_SCOPE)

	macro(LUA_LINK target)
		add_dependencies(${target} luajit)
		link_directories(${INSTALL_DIR}/lib)
		include_directories(${LUA_INCLUDE_DIR})
		target_link_libraries(${target} ${LUA_LIBRARY})
	endmacro(LUA_LINK)
else(USE_LUAJIT)
	macro(LUA_LINK target)
		find_package(Lua51 REQUIRED)
		include_directories(${LUA_INCLUDE_DIR})
		target_link_libraries(${target} ${LUA_LIBRARIES})
	endmacro(LUA_LINK)
endif(USE_LUAJIT)

