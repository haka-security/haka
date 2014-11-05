# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(LibCapstone REQUIRED)
if(LIBCAPSTONE_FOUND)
	swig_process(asm lua asm.i)

	add_library(asm MODULE asm.c
		${SWIG_asm_FILES})

	SWIG_FIX_ENTRYPOINT(asm misc)

	include_directories(${LIBCAPSTONE_INCLUDE_DIR})
	target_link_libraries(asm ${LIBCAPSTONE_LIBRARY})

	INSTALL_MODULE(asm misc)

	add_subdirectory(test)
else()
    message(STATUS "Not building module asm (missing libraries)")
endif()
