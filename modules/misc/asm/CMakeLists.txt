# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

swig_process(asm lua asm.i)

add_library(asm MODULE asm.c
	${SWIG_asm_FILES})

MODULE_FIX_LUA_ENTRYPOINT(asm misc)

include_directories(${LIBCAPSTONE_INCLUDE_DIR})
target_link_libraries(asm LINK_PRIVATE ${LIBCAPSTONE_LIBRARY})

if(LIBCAPSTONE_DEPENDENCY)
	add_dependencies(asm ${LIBCAPSTONE_DEPENDENCY})
endif()

INSTALL_MODULE(asm misc)

add_subdirectory(test)
