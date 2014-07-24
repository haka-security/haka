# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_INSTALL_RPATH "${CMAKE_HAKA_INSTALL_PREFIX}/lib")
set(MODULE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

macro(SWIG_FIX_ENTRYPOINT target type)
	set_target_properties(${target} PROPERTIES LINK_FLAGS "-Wl,--defsym=luaopen_${type}/${target}=luaopen_${target}")
endmacro()

# Setup module file names
set(CMAKE_SHARED_MODULE_PREFIX "${HAKA_MODULE_PREFIX}")
set(CMAKE_SHARED_MODULE_SUFFIX "${HAKA_MODULE_SUFFIX}")
set(CMAKE_SHARED_LIBRARY_PREFIX "${HAKA_MODULE_PREFIX}")
set(CMAKE_SHARED_LIBRARY_SUFFIX "${HAKA_MODULE_SUFFIX}")

set(MODULE_INSTALL_PATH ${HAKA_INSTALL_PREFIX}/share/haka/modules)
set(MODULE_INSTALL_CPATH ${HAKA_INSTALL_PREFIX}/lib/haka/modules)

macro(INCLUDE_MODULE name type)
	include_directories(${MODULE_SOURCE_DIR}/${type}/${name})
endmacro(INCLUDE_MODULE)

macro(DEPENDS_MODULE target name type)
	INCLUDE_MODULE(${name} ${type})
	target_link_libraries(${target} ${name})
	set_target_properties(${target} PROPERTIES INSTALL_RPATH "${CMAKE_HAKA_INSTALL_PREFIX}/share/haka/modules/${type}")
endmacro(DEPENDS_MODULE)

macro(INSTALL_MODULE target type)
	set_property(GLOBAL APPEND PROPERTY module-${type} ${target})
	include_directories(.)
	target_link_libraries(${target} libhaka)
	install(TARGETS ${target} LIBRARY DESTINATION ${MODULE_INSTALL_CPATH}/${type})
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
