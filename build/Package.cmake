# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# CPack default Packaging
set(CPACK_GENERATOR "TGZ")
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_PACKAGE_RELOCATABLE FALSE)
set(CPACK_PACKAGE_NAME "haka")
set(CPACK_PACKAGE_VENDOR "Arkoon Network Security")
set(CPACK_PACKAGE_VERSION_MAJOR ${HAKA_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HAKA_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HAKA_VERSION_PATCH})

if(${HAKA_VERSION_PATCH} GREATER 0)
	set(CPACK_PACKAGE_VERSION "${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}${HAKA_VERSION_BUILD}")
else()
	set(CPACK_PACKAGE_VERSION "${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}${HAKA_VERSION_BUILD}")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} build)
set(CPACK_PACKAGE_FULLNAME "${CPACK_PACKAGE_NAME}-${build}-${CMAKE_LUA}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FULLNAME}_${CPACK_PACKAGE_VERSION}_${CMAKE_SYSTEM_PROCESSOR}")

# Sources
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR};\\\\.git/;\\\\.git$;\\\\.gitignore$;\\\\.gitmodules$;.*\\\\.swp;.*\\\\.pyc;__pycache__/;/make/;/workspace/")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_source")

include(CPack)

find_program(SHA1SUM_COMMAND sha1sum)
if(NOT SHA1SUM_COMMAND)
	message(WARNING "Cannot find sha1sum command")
else()
	add_custom_target(package_source_sha
		COMMAND $(MAKE) package_source
		COMMAND sha1sum ${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_source.tar.gz > ${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_source.tar.gz.sha1.txt
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Build source package" VERBATIM
	)
endif()
