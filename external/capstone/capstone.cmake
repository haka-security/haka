# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CAPSTONE_DIR "external/capstone")

find_package(Rsync REQUIRED)

add_custom_target(capstone-sync
	COMMAND ${RSYNC_EXECUTABLE} -rt ${CMAKE_SOURCE_DIR}/${CAPSTONE_DIR}/src/ ${CMAKE_BINARY_DIR}/${CAPSTONE_DIR}
)

add_custom_command(OUTPUT "${CAPSTONE_DIR}/libcapstone.a"
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${CAPSTONE_DIR}
	DEPENDS capstone-sync
)

add_custom_target(capstone
	DEPENDS "${CAPSTONE_DIR}/libcapstone.a"
)

set(LIBCAPSTONE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/${CAPSTONE_DIR}/src/include/")
set(LIBCAPSTONE_LIBRARY "${CMAKE_BINARY_DIR}/${CAPSTONE_DIR}/libcapstone.a")
