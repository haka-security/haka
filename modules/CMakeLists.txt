# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(MODULE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

macro(MODULE_FIX_ENTRYPOINT name type)
	set_target_properties(${name} PROPERTIES LINK_FLAGS "-Wl,--defsym=luaopen_${type}/${name}=luaopen_${name}")
endmacro()

macro(ADD_MODULE)
	set(oneValueArgs NAME TYPE)
	set(multiValueArgs FILES LUA)
	cmake_parse_arguments(MODULE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	foreach(file ${MODULE_LUA})
		get_filename_component(file_dir "${file}" PATH)
		get_filename_component(file_path "${file}" ABSOLUTE)
		get_filename_component(file_fullname "${file}" NAME)
		get_filename_component(file_name "${file}" NAME_WE)

		set(lua_preprocessed_file "${file_dir}/${file_name}.lua")
		add_custom_command(
			OUTPUT "${lua_preprocessed_file}"
			COMMAND mkdir ${file_dir}
			COMMAND ${CMAKE_C_COMPILER} -E -P -x c ${file_path} -o ${lua_preprocessed_file}
			COMMENT "Preprocessing Lua file ${file}"
			DEPENDS "${file_path}"
			VERBATIM)

		set(lua_bytecode_file "${file_dir}/${file_name}.bc")
		add_custom_command(
			OUTPUT "${lua_bytecode_file}"
			COMMAND ${LUA_COMPILER} ${LUA_FLAGS} -o ${lua_bytecode_file} ${lua_preprocessed_file}
			COMMENT "Building Lua file ${lua_preprocessed_file}"
			DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${lua_preprocessed_file}" ${LUA_DEPENDENCY}
			VERBATIM)

		set(lua_header_file "${file_dir}/${file_name}.h")
		add_custom_command(
			OUTPUT "${lua_header_file}"
			COMMAND ${LUA_BIN} ${BC2C} ${file_name} ${CMAKE_CURRENT_BINARY_DIR}/${lua_bytecode_file} ${CMAKE_CURRENT_BINARY_DIR}/${lua_header_file}
			DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${lua_bytecode_file}"
			COMMENT "Building C header from Lua file ${lua_bytecode_file}"
			VERBATIM)

		LIST(APPEND BINDING_FILES "${CMAKE_CURRENT_BINARY_DIR}/${lua_header_file}")
	endforeach(file)

	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	add_library(${MODULE_NAME} SHARED ${MODULE_FILES} ${BINDING_FILES})

	MODULE_FIX_ENTRYPOINT(${MODULE_NAME} ${MODULE_TYPE})
endmacro()

# Setup module file names
set(CMAKE_SHARED_MODULE_PREFIX "${HAKA_MODULE_PREFIX}")
set(CMAKE_SHARED_MODULE_SUFFIX "${HAKA_MODULE_SUFFIX}")
set(CMAKE_SHARED_LIBRARY_PREFIX "${HAKA_MODULE_PREFIX}")
set(CMAKE_SHARED_LIBRARY_SUFFIX "${HAKA_MODULE_SUFFIX}")

set(MODULE_INSTALL_PATH share/haka/modules)
set(MODULE_INSTALL_CPATH lib/haka/modules)
set(MODULE_INSTALL_LIB lib/haka)

macro(INCLUDE_MODULE name type)
	include_directories(${MODULE_SOURCE_DIR}/${type}/${name})
endmacro(INCLUDE_MODULE)

macro(DEPENDS_MODULE target name type)
	INCLUDE_MODULE(${name} ${type})
	target_link_libraries(${target} LINK_PUBLIC ${name})
	set_property(TARGET ${target} APPEND PROPERTY INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${MODULE_INSTALL_CPATH}/${type}")
endmacro(DEPENDS_MODULE)

macro(INSTALL_MODULE target type)
	set_property(GLOBAL APPEND PROPERTY module-${type} ${target})
	include_directories(.)
	target_link_libraries(${target} LINK_PUBLIC libhaka)
	install(TARGETS ${target} LIBRARY DESTINATION ${MODULE_INSTALL_CPATH}/${type} ${ARGN})
	set_property(TARGET ${target} APPEND PROPERTY INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${MODULE_INSTALL_LIB}")
endmacro(INSTALL_MODULE)

# Find all modules. We need a separated loop in case one of the sub make adds
# some tests
set(MODULES_BINARY_DIRS "")
set(MODULES_SOURCE_DIRS "")
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*/*/CMakeLists.txt")
foreach(subdir ${SUBDIRS})
	get_filename_component(DIR ${CMAKE_CURRENT_BINARY_DIR}/${subdir} PATH)
	set(MODULES_BINARY_DIRS "${DIR}:${MODULES_BINARY_DIRS}")
	get_filename_component(DIR ${CMAKE_CURRENT_SOURCE_DIR}/${subdir} PATH)
	set(MODULES_SOURCE_DIRS "${DIR}:${MODULES_SOURCE_DIRS}")
endforeach(subdir)
set(MODULES_BINARY_DIRS "${MODULES_BINARY_DIRS}" CACHE INTERNAL "list of build directories for the modules")
set(MODULES_SOURCE_DIRS "${MODULES_SOURCE_DIRS}" CACHE INTERNAL "list of source directories for the modules")

# Add all sub directories
foreach(subdir ${SUBDIRS})
	get_filename_component(DIR ${CMAKE_CURRENT_SOURCE_DIR}/${subdir} PATH)
	add_subdirectory(${DIR})
endforeach(subdir)
