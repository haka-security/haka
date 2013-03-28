
# Setup module file names
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".ho")

# Add all sub directories
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(subdir ${SUBDIRS})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
		if(NOT ${subdir} MATCHES "CMakeFiles")
			add_subdirectory(${subdir})
		endif()
	endif()
endforeach(subdir)

