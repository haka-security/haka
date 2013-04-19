
set(CMAKE_INSTALL_RPATH "$ORIGIN/..")

# Setup module file names
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".ho")

# Add all sub directories
set(MODULES_BINARY_DIRS "")
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(subdir ${SUBDIRS})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
		if(NOT ${subdir} MATCHES "CMakeFiles")
			add_subdirectory(${subdir})
			set(MODULES_BINARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}/${subdir}:${MODULES_BINARY_DIRS}")
		endif()
	endif()
endforeach(subdir)
set(MODULES_BINARY_DIRS "${MODULES_BINARY_DIRS}" CACHE INTERNAL "list of directories containing modules")
