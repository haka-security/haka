
if(LUA STREQUAL "luajit")
	set(HAKA_LUAJIT 1)
	set(HAKA_LUA51 1)
	set(LUA_DEPENDS luajit)
elseif(LUA STREQUAL "lua51")
	find_package(Lua51 REQUIRED)
	set(HAKA_LUAJIT 0)
	set(HAKA_LUA51 1)
elseif(LUA STREQUAL "custom")
else()
	message(FATAL_ERROR "Invalid Lua version")
endif()

macro(LUA_LINK target)
	if (LUA_DEPENDS)
		add_dependencies(${target} ${LUA_DEPENDS})
	endif(LUA_DEPENDS)

	include_directories(${LUA_INCLUDE_DIR})
	link_directories(${LUA_LIBRARY_DIR})
	target_link_libraries(${target} ${LUA_LIBRARIES})
endmacro(LUA_LINK)

macro(LUA_INSTALL)
	set(options COMPILED)
	set(oneValueArgs DESTINATION NAME)
	set(multiValueArgs FILES)
	cmake_parse_arguments(LUA_INSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if (LUA_INSTALL_COMPILED)
		set(LUA_INSTALL_COMPILED_FILES "")

		foreach(it ${LUA_INSTALL_FILES})
			get_filename_component(lua_source_file_path "${it}" ABSOLUTE)
			get_filename_component(lua_source_file_name "${it}" NAME_WE)
			set(lua_source_outfile_path "${lua_source_file_name}.bc")

			add_custom_command(
				OUTPUT "${lua_source_outfile_path}"
				COMMAND ${LUA_COMPILER} ${LUA_FLAGS_${CMAKE_BUILD_TYPE}} -o ${lua_source_outfile_path} ${lua_source_file_path}
				MAIN_DEPENDENCY "${lua_source_file_path}"
				COMMENT "Building Lua file ${it}"
				VERBATIM)

			SET_SOURCE_FILES_PROPERTIES("${lua_source_outfile_path}" PROPERTIES GENERATED 1)
			LIST(APPEND LUA_INSTALL_COMPILED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${lua_source_outfile_path}")
		endforeach(it)

		add_custom_target(${LUA_INSTALL_NAME} ALL DEPENDS ${LUA_INSTALL_COMPILED_FILES} ${LUA_DEPENDS})

		GET_DIRECTORY_PROPERTY(lua_extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
		SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${lua_extra_clean_files};${LUA_INSTALL_COMPILED_FILES}")

		install(FILES ${LUA_INSTALL_COMPILED_FILES} DESTINATION ${LUA_INSTALL_DESTINATION})
	else()
		install(FILES ${LUA_INSTALL_FILES} DESTINATION ${LUA_INSTALL_DESTINATION})
	endif()

endmacro(LUA_INSTALL)
