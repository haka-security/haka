
set(CMAKE_INSTALL_RPATH "$ORIGIN/..")


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
