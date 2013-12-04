# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Sphinx)
if(SPHINX_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY)
	add_custom_target(doc
		ALL
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/doc
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating user guide" VERBATIM
	)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/ DESTINATION ${HAKA_INSTALL_PREFIX}/share/doc/haka/manual)

endif(SPHINX_FOUND)
