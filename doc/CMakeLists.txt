# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Sphinx)
find_package(Doxygen)

if(SPHINX_FOUND AND DOXYGEN_FOUND)
	# The tool to rebuild doc images are optional
	find_package(Inkscape)
	find_package(Blockdiag)
	find_package(Seqdiag)
	find_package(Actdiag)
	find_package(Packetdiag)

	# Macro to convert diagram to png images
	macro(generate_images)
		set(oneValueArgs TARGET TOOL)
		set(multiValueArgs FILES)
		cmake_parse_arguments(generate_images "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

		set(generate_images_OUT)
		string(TOUPPER "${generate_images_TOOL}" generate_images_TOOL_UPPER)
		set(generate_images_EXECUTABLE "${generate_images_TOOL_UPPER}_EXECUTABLE")

		if(${generate_images_TOOL_UPPER}_FOUND AND INKSCAPE_FOUND)
			foreach(it ${generate_images_FILES})
				get_filename_component(source_file "${it}" ABSOLUTE)
				get_filename_component(source_file_path "${it}" PATH)
				get_filename_component(source_file_fullname "${it}" NAME)
				get_filename_component(source_file_name "${it}" NAME_WE)
				set(dest_path "${CMAKE_CURRENT_SOURCE_DIR}/genimages/${source_file_path}")
				set(dest_file "${dest_path}/${source_file_name}.png")

				if("${generate_images_EXECUTABLE}" STREQUAL "INKSCAPE_EXECUTABLE")
					add_custom_command(
						OUTPUT "${dest_file}"
						COMMAND mkdir -p ${dest_path}
						COMMAND ${INKSCAPE_EXECUTABLE} ${source_file} -e ${dest_file} > /dev/null
						MAIN_DEPENDENCY "${source_file}"
						COMMENT "Building image for ${it}")
				else()
					set(dest_svg_file "${source_file_path}/${source_file_name}.svg")

					add_custom_command(
						OUTPUT "${dest_file}"
						COMMAND mkdir -p ${dest_path}
						COMMAND mkdir -p ${source_file_path}
						COMMAND ${${generate_images_EXECUTABLE}} -T SVG -o ${dest_svg_file} ${source_file}
						COMMAND ${INKSCAPE_EXECUTABLE} ${dest_svg_file} -d 110 -e ${dest_file} > /dev/null
						MAIN_DEPENDENCY "${source_file}"
						COMMENT "Building image for ${it}")
				endif()

				LIST(APPEND generate_images_OUT "${dest_file}")
			endforeach(it)

			if(TARGET ${generate_images_TARGET})
				get_target_property(ID ${generate_images_TARGET} ID)
				MATH(EXPR ID "${ID}+1")
				set_target_properties(${generate_images_TARGET} PROPERTIES ID ${ID})
				add_custom_target(${generate_images_TARGET}-${ID} DEPENDS ${generate_images_OUT})
				add_dependencies(${generate_images_TARGET} ${generate_images_TARGET}-${ID})
			else()
				add_custom_target(${generate_images_TARGET} DEPENDS ${generate_images_OUT})
				set_target_properties(${generate_images_TARGET} PROPERTIES ID "0")
			endif()
		else()
			if(NOT ${generate_images_TOOL_UPPER}_FOUND)
				message("Images not generated (${generate_images_TOOL} missing)")
			else()
				message("Images not generated (Inkscape missing)")
			endif()

			if(NOT TARGET ${generate_images_TARGET})
				# Add empty target needed by dependencies
				add_custom_target(${generate_images_TARGET})
				set_target_properties(${generate_images_TARGET} PROPERTIES ID "0")
			endif()
		endif()

		unset(generate_images_TARGET)
		unset(generate_images_TOOL)
		unset(generate_images_FILES)
		unset(generate_images_EXECUTABLE)
		unset(generate_images_OUT)
	endmacro(generate_images)

	# Configure files
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

	# Images
	generate_images(TARGET doc-images TOOL Inkscape FILES
		developer/flow.svg
		developer/arch.svg
		ref/buffer.svg
		ref/subbuffer.svg
		ref/bufferstream.svg
	)

	generate_images(TARGET doc-images TOOL Seqdiag FILES
		developer/capture.diag
		developer/dissector.diag
		developer/flowdiss.diag
	)
	generate_images(TARGET doc-images TOOL Blockdiag FILES
		developer/ref/vbuffer.diag
		developer/ref/vbuffer_iterator.diag
		developer/ref/vbuffer_sub.diag
		developer/ref/vbuffer_split.diag
		developer/ref/vbuffer_mark.diag
	)

	# Real doc target
	add_custom_target(doc-doxygen
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating Doxygen documentation" VERBATIM
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile doc-images
	)

	# Build the sphinx documentation without regenerating the Doxygen doc
	# This target is meant for development use
	add_custom_target(doc-sphinx
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/doc
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating Sphinx documentation" VERBATIM
		DEPENDS doc-images
	)

	add_custom_target(doc
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/doc
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating Sphinx documentation" VERBATIM
		DEPENDS doc-doxygen doc-images
	)

	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/ DESTINATION share/doc/haka/manual OPTIONAL)
else()
	message("Skipping documentation target (Sphinx and Doxygen not found)")
endif()

add_subdirectory(test)
