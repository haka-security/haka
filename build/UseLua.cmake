# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(LUA STREQUAL "luajit")
	include(external/luajit.cmake)
elseif(LUA STREQUAL "lua")
	include(external/lua.cmake)
else()
	message(FATAL_ERROR "Invalid Lua version")
endif()

macro(LUA_DEPENDS target)
	if (LUA_DEPENDENCY)
		add_dependencies(${target} ${LUA_DEPENDENCY})
	endif(LUA_DEPENDENCY)
endmacro(LUA_DEPENDS)

macro(LUA_LINK target)
	LUA_DEPENDS(${target})
	include_directories(${LUA_INCLUDE_DIR})
	link_directories(${LUA_LIBRARY_DIR})
	target_link_libraries(${target} ${LUA_LIBRARIES})
endmacro(LUA_LINK)

macro(LUA_COMPILE)
	set(oneValueArgs NAME)
	set(multiValueArgs FILES FLAGS)
	cmake_parse_arguments(LUA_COMPILE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(ARGS_LIST "${ARGN}")
	list(FIND ARGS_LIST "FLAGS" LUA_COMPILE_HAS_FLAGS)

	if(LUA_COMPILE_HAS_FLAGS EQUAL -1)
		string(TOUPPER "LUA_FLAGS_${CMAKE_BUILD_TYPE}" LUA_FLAGS)
		set(LUA_FLAGS "${${LUA_FLAGS}}")
	else()
		set(LUA_FLAGS "${LUA_COMPILE_FLAGS}")
	endif()

	foreach(it ${LUA_COMPILE_FILES})
		get_filename_component(lua_source_file_path "${it}" ABSOLUTE)
		get_filename_component(lua_source_file_name "${it}" NAME_WE)
		set(lua_source_outfile_path "${lua_source_file_name}.bc")

		add_custom_command(
			OUTPUT "${lua_source_outfile_path}"
			COMMAND ${LUA_COMPILER} ${LUA_FLAGS} -o ${lua_source_outfile_path} ${lua_source_file_path}
			MAIN_DEPENDENCY "${lua_source_file_path}"
			COMMENT "Building Lua file ${it}"
			DEPENDS ${LUA_DEPENDENCY}
			VERBATIM)

		SET_SOURCE_FILES_PROPERTIES("${lua_source_outfile_path}" PROPERTIES GENERATED 1)
		LIST(APPEND LUA_COMPILE_COMPILED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${lua_source_outfile_path}")
	endforeach(it)

	get_directory_property(lua_extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${lua_extra_clean_files};${LUA_COMPILE_COMPILED_FILES}")

	if(TARGET ${LUA_COMPILE_NAME})
		get_target_property(ID ${LUA_COMPILE_NAME} LUA_ID)
		set_target_properties(${LUA_COMPILE_NAME} PROPERTIES LUA_ID ${LUA_ID}+1)

		add_custom_target(${LUA_COMPILE_NAME}-${ID} ALL DEPENDS ${LUA_COMPILE_COMPILED_FILES} ${LUA_DEPENDS})

		add_dependencies(${LUA_COMPILE_NAME} ${LUA_COMPILE_NAME}-${ID})
		get_target_property(FILES ${LUA_COMPILE_NAME} LUA_FILES)
		list(APPEND FILES "${LUA_COMPILE_COMPILED_FILES}")
		set_target_properties(${LUA_COMPILE_NAME} PROPERTIES LUA_FILES "${FILES}")
	else()
		add_custom_target(${LUA_COMPILE_NAME} ALL DEPENDS ${LUA_COMPILE_COMPILED_FILES} ${LUA_DEPENDS})
		set_target_properties(${LUA_COMPILE_NAME} PROPERTIES LUA_FILES "${LUA_COMPILE_COMPILED_FILES}")
		set_target_properties(${LUA_COMPILE_NAME} PROPERTIES LUA_ID "0")
	endif()

endmacro(LUA_COMPILE)

macro(LUA_INSTALL)
	set(oneValueArgs DESTINATION TARGET)
	cmake_parse_arguments(LUA_INSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	get_target_property(FILES ${LUA_INSTALL_TARGET} LUA_FILES)
	install(FILES ${FILES} DESTINATION ${LUA_INSTALL_DESTINATION})
endmacro(LUA_INSTALL)
