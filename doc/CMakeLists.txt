
find_package(Sphinx)
if(SPHINX_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/userguide/conf.py @ONLY)
	add_custom_target(doc-userguide
		ALL
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR}/userguide ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/userguide/doc
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/userguide
		COMMENT "Generating user guide" VERBATIM
	)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/userguide DESTINATION ${HAKA_INSTALL_PREFIX}/share/doc/haka/userguide/doc)

	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/confdev.py.in ${CMAKE_CURRENT_BINARY_DIR}/developerguide/conf.py @ONLY)
	add_custom_target(doc-developerguide
		ALL
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR}/developerguide ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/developerguide/doc
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/developerguide
		COMMENT "Generating developer guide" VERBATIM
	)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/developerguide DESTINATION ${HAKA_INSTALL_PREFIX}/share/doc/haka/developerguide/doc)

	add_custom_target(doc DEPENDS doc-userguide doc-developerguide)
endif(SPHINX_FOUND)
