# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(UseBc2c)
include(UseLuaTemplate)

if(CMAKE_LUA STREQUAL "luajit")
	include(external/luajit/luajit.cmake)
elseif(CMAKE_LUA STREQUAL "lua")
	include(external/lua/lua.cmake)
else()
	message(FATAL_ERROR "Invalid Lua version")
endif()


set(LUAC "yes" CACHE BOOL "precompile lua files during build")


include_directories(${LUA_INCLUDE_DIR})

macro(LUA_COMPILE)
	set(options PREPROCESS)
	set(oneValueArgs NAME)
	set(multiValueArgs FILES FLAGS)
	cmake_parse_arguments(LUA_COMPILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(ARGS_LIST "${ARGN}")
	list(FIND ARGS_LIST "FLAGS" LUA_COMPILE_HAS_FLAGS)

	if(LUA_COMPILE_HAS_FLAGS EQUAL -1)
		string(TOUPPER "LUA_FLAGS_${CMAKE_BUILD_TYPE}" LUA_FLAGS)
		set(LUA_FLAGS "${${LUA_FLAGS}}")
	else()
		set(LUA_FLAGS "${LUA_COMPILE_FLAGS}")
	endif()

	set(LUA_COMPILE_COMPILED_FILES)

	foreach(it ${LUA_COMPILE_FILES})
		get_filename_component(lua_source_file_path "${it}" ABSOLUTE)
		get_filename_component(lua_source_file_name "${it}" NAME_WE)
		get_filename_component(lua_source_file_dir  "${it}" PATH)

		set(lua_source_outfile_path "${lua_source_file_name}.bc")

		if(LUA_COMPILE_PREPROCESS)
			get_directory_property(inc INCLUDE_DIRECTORIES)
			if (inc)
				foreach(dir ${inc})
					list(APPEND inc_flags "-I${dir}")
				endforeach()
			endif()
			set(lua_preprocessed_file "${CMAKE_CURRENT_BINARY_DIR}/${lua_source_file_dir}/${lua_source_file_name}.lua")
			add_custom_command(
				OUTPUT "${lua_preprocessed_file}"
				COMMAND mkdir -p ${lua_source_file_dir}
				COMMAND ${CMAKE_C_COMPILER} ${inc_flags} -E -P -x c ${lua_source_file_path} -o ${lua_preprocessed_file}
				COMMENT "Preprocessing Lua file ${it}"
				DEPENDS "${lua_source_file_path}"
				IMPLICIT_DEPENDS C ${lua_source_file_path}
				VERBATIM)
		else()
			set(lua_preprocessed_file "${lua_source_file_path}")
		endif()

		if(LUAC)
			add_custom_command(
				OUTPUT "${lua_source_outfile_path}"
				COMMAND ${LUA_COMPILER} ${LUA_FLAGS} -o ${lua_source_outfile_path} ${lua_preprocessed_file}
				MAIN_DEPENDENCY "${lua_preprocessed_file}"
				COMMENT "Building Lua file ${it}"
				DEPENDS ${LUA_DEPENDENCY}
				VERBATIM)
		endif()

		SET_SOURCE_FILES_PROPERTIES("${lua_source_outfile_path}" PROPERTIES GENERATED 1)
		LIST(APPEND LUA_COMPILE_COMPILED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${lua_source_outfile_path}")
	endforeach(it)

	get_directory_property(lua_extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${lua_extra_clean_files};${LUA_COMPILE_COMPILED_FILES}")

	if(TARGET ${LUA_COMPILE_NAME})
		get_target_property(ID ${LUA_COMPILE_NAME} LUA_ID)
		MATH(EXPR ID "${ID}+1")
		set_target_properties(${LUA_COMPILE_NAME} PROPERTIES LUA_ID ${ID})

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

macro(BINDING_COMPILE)
	set(oneValueArgs NAME)
	set(multiValueArgs FILES LUA)
	cmake_parse_arguments(BINDING "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(BINDING_COMPILED_FILES)

	foreach(file ${BINDING_LUA})
		get_filename_component(file_dir "${file}" PATH)
		get_filename_component(file_path "${file}" ABSOLUTE)
		get_filename_component(file_fullname "${file}" NAME)
		get_filename_component(file_name "${file}" NAME_WE)

		get_directory_property(inc INCLUDE_DIRECTORIES)
		if (inc)
			foreach(dir ${inc})
				list(APPEND inc_flags "-I${dir}")
			endforeach()
		endif()
		set(lua_preprocessed_file "${file_dir}/${file_name}.lua")
		add_custom_command(
			OUTPUT "${lua_preprocessed_file}"
			COMMAND mkdir -p ${file_dir}
			COMMAND ${CMAKE_C_COMPILER} ${inc_flags} -E -P -x c ${file_path} -o ${lua_preprocessed_file}
			COMMENT "Preprocessing Lua file ${file}"
			DEPENDS "${file_path}"
			IMPLICIT_DEPENDS C ${file_path}
			VERBATIM)

		if (LUAC)
			set(lua_bytecode_file "${file_dir}/${file_name}.bc")
			add_custom_command(
				OUTPUT "${lua_bytecode_file}"
				COMMAND ${LUA_COMPILER} ${LUA_FLAGS} -o ${lua_bytecode_file} ${lua_preprocessed_file}
				COMMENT "Building Lua file ${lua_preprocessed_file}"
				DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${lua_preprocessed_file}" ${LUA_DEPENDENCY}
				VERBATIM)
		else()
			set(lua_bytecode_file "${lua_preprocessed_file}")
		endif()

		set(lua_header_file "${file_dir}/${file_name}.h")
		add_custom_command(
			OUTPUT "${lua_header_file}"
			COMMAND ${LUA_BIN} ${BC2C} ${file_name} ${CMAKE_CURRENT_BINARY_DIR}/${lua_bytecode_file} ${CMAKE_CURRENT_BINARY_DIR}/${lua_header_file}
			DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${lua_bytecode_file}"
			COMMENT "Building C header from Lua file ${lua_bytecode_file}"
			VERBATIM)

		LIST(APPEND BINDING_COMPILED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${lua_header_file}")
	endforeach(file)

	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	set(BINDING_${BINDING_NAME}_FILES ${BINDING_FILES} ${BINDING_COMPILED_FILES})
endmacro(BINDING_COMPILE)

macro(TEMPLATE_COMPILE)
	set(oneValueArgs NAME)
	set(multiValueArgs FILES)
	cmake_parse_arguments(TEMPLATE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(TEMPLATE_COMPILED_FILES)

	foreach(file ${TEMPLATE_FILES})
		get_filename_component(file_dir "${file}" PATH)
		get_filename_component(file_path "${file}" ABSOLUTE)

		string(REGEX REPLACE ".([^.]+).tmpl$" "_\\1.lua" precompiled_file "${file}")
		add_custom_command(
			OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${precompiled_file}"
			COMMAND mkdir -p ${file_dir}
			COMMAND ${LUA_BIN} ${LUA_TEMPLATE_COMPILER} "${file_path}" "${CMAKE_CURRENT_BINARY_DIR}/${precompiled_file}"
			DEPENDS "${file_path}" "${LUA_TEMPLATE_COMPILER}"
			IMPLICIT_DEPENDS C "${file_path}"
			COMMENT "Precompile Lua template file ${file}")
		LIST(APPEND TEMPLATE_COMPILED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${precompiled_file}")
	endforeach(file)

	set(TEMPLATE_${TEMPLATE_NAME}_FILES ${TEMPLATE_COMPILED_FILES})
endmacro(TEMPLATE_COMPILE)
