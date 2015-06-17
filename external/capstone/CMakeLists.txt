# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

unset(LIBCAPSTONE_LIBRARY CACHE)
unset(LIBCAPSTONE_INCLUDE_DIR CACHE)
unset(LIBCAPSTONE_DEPENDENCY CACHE)

find_package(LibCapstone)

if(NOT ${LIBCAPSTONE_FOUND})
    message(STATUS "Building static capstone library from externals")

	find_package(Rsync REQUIRED)

	add_custom_target(capstone-sync
		COMMAND ${RSYNC_EXECUTABLE} -rt ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_BINARY_DIR})

	add_custom_command(OUTPUT "libcapstone.a"
		COMMAND make
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS capstone-sync
	)

	add_custom_target(capstone
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/libcapstone.a"
	)

	set(LIBCAPSTONE_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/libcapstone.a" CACHE INTERNAL "capstone libaray")
	set(LIBCAPSTONE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/include" CACHE INTERNAL "capstone include dir")
	set(LIBCAPSTONE_DEPENDENCY capstone CACHE INTERNAL "capstone dependency")
endif()
