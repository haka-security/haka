
set(CMAKE_INSTALL_RPATH "$ORIGIN/../../lib")

# Setup module file names
set(CMAKE_SHARED_MODULE_PREFIX "${HAKA_MODULE_PREFIX}")
set(CMAKE_SHARED_MODULE_SUFFIX "${HAKA_MODULE_SUFFIX}")
set(CMAKE_SHARED_LIBRARY_PREFIX "${HAKA_MODULE_PREFIX}")
set(CMAKE_SHARED_LIBRARY_SUFFIX "${HAKA_MODULE_SUFFIX}")

macro(INCLUDE_MODULE name)
	include_directories(${CMAKE_SOURCE_DIR}/modules/${name})
endmacro(INCLUDE_MODULE)

macro(DEPENDS_MODULE target name)
	INCLUDE_MODULE(name)
	target_link_libraries(${target} ${name})
endmacro(DEPENDS_MODULE)

macro(INSTALL_MODULE target)
	include_directories(.)
	target_link_libraries(${target} libhaka)
	install(TARGETS ${target} LIBRARY DESTINATION bin/modules)
endmacro(INSTALL_MODULE)

# Find all modules. We need a separated loop in case one of the sub make adds
# some tests
set(MODULES_BINARY_DIRS "")
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(subdir ${SUBDIRS})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
		set(MODULES_BINARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}/${subdir}:${MODULES_BINARY_DIRS}")
	endif()
endforeach(subdir)
set(MODULES_BINARY_DIRS "${MODULES_BINARY_DIRS}" CACHE INTERNAL "list of directories containing modules")

# Add all sub directories
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(subdir ${SUBDIRS})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
		if(NOT ${subdir} MATCHES "CMakeFiles")
			add_subdirectory(${subdir})
		endif()
	endif()
endforeach(subdir)
