# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(PCRE REQUIRED)
if(PCRE_FOUND)
	swig_process(PCRE lua pcre.i)

	add_library(pcre MODULE main.c ${SWIG_PCRE_FILES})

	SWIG_FIX_ENTRYPOINT(pcre regexp)

	include_directories(${PCRE_INCLUDE_DIR})
	target_link_libraries(pcre ${PCRE_LIBRARY})

	LUA_LINK(pcre)

	INSTALL_MODULE(pcre regexp)

	# Tests
	add_subdirectory(test)
endif()
