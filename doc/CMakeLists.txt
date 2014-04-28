# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Sphinx REQUIRED)
find_package(Doxygen REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

# Real doc target
add_custom_target(doxygen
	COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating Doxygen documentation" VERBATIM
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
)

add_custom_target(doc
	COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/doc
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating Sphinx documentation" VERBATIM
	DEPENDS doxygen
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/ DESTINATION ${HAKA_INSTALL_PREFIX}/share/doc/haka/manual OPTIONAL)

add_subdirectory(test)
