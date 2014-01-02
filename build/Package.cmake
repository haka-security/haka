# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Packaging
set(CPACK_GENERATOR ${DISTRIB_PACKAGE})
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_PACKAGE_RELOCATABLE FALSE)
set(CPACK_PACKAGE_NAME "haka")
set(CPACK_PACKAGE_VENDOR "Arkoon Network Security")
set(CPACK_PACKAGE_VERSION "${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}${HAKA_VERSION_BUILD}")
set(CPACK_PACKAGE_VERSION_MAJOR ${HAKA_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HAKA_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HAKA_VERSION_PATCH})

if(BUILDTAG STREQUAL "official")
    set(CPACK_PACKAGE_FULLNAME "${CPACK_PACKAGE_NAME}")
else()
    set(CPACK_PACKAGE_FULLNAME "${CPACK_PACKAGE_NAME}-${LUA}")
endif()

if(DISTRIB_PACKAGE STREQUAL "DEB")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FULLNAME}_${CPACK_PACKAGE_VERSION}_${CMAKE_SYSTEM_PROCESSOR}")
elseif(DISTRIB_PACKAGE STREQUAL "RPM")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FULLNAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Sources
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR};\\\\.git/;\\\\.git$;\\\\.gitignore$;\\\\.gitmodules$;.*\\\\.swp;.*\\\\.pyc;__pycache__/")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FULLNAME}_${CPACK_PACKAGE_VERSION}")

# Common
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Haka runtime is a tool that allows to capture network traffic and filter them based on Lua policy files.")
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackEnv.cmake")
FILE(WRITE "${CPACK_PROJECT_CONFIG_FILE}" "set(ENV{LD_LIBRARY_PATH} \"${CMAKE_CURRENT_BINARY_DIR}/_CPack_Packages/\${CPACK_TOPLEVEL_TAG}/DEB/\${CPACK_PACKAGE_FILE_NAME}/${HAKA_INSTALL_PREFIX}/lib\")\n")
install(FILES LICENSE.txt DESTINATION ${HAKA_INSTALL_PREFIX}/share/doc/haka)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.txt)

# Debian package
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Arkoon Network Security")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/src/haka/init/postinst;${CMAKE_BINARY_DIR}/src/haka/init/postrm;${CMAKE_BINARY_DIR}/src/haka/init/preinst;${CMAKE_BINARY_DIR}/src/haka/init/prerm;${CMAKE_BINARY_DIR}/src/haka/init/conffiles;")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.haka-security.org")


# RPM package
set(CPACK_RPM_SPEC_MORE_DEFINE
"%global _privatelibs libhaka[.]so.*
%global _privatelibs %{_privatelibs}|libluajit-5.1[.]so.*
%global _privatelibs %{_privatelibs}|liblua[.]so.*
%global __provides_exclude ^(%{_privatelibs})$
%global __requires_exclude ^(%{_privatelibs})$")
set(CPACK_RPM_PACKAGE_LICENSE "Mozilla Public License Version 2.0")
set(CPACK_RPM_PACKAGE_GROUP "Security")
set(CPACK_RPM_PACKAGE_URL "http://www.haka-security.org")
set(CPACK_RPM_USER_FILELIST "%config ${CMAKE_HAKA_INSTALL_PREFIX}/etc/haka/haka.conf")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/src/haka/init/postinst")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/src/haka/init/postrm")
set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/src/haka/init/preinst")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/src/haka/init/prerm")

include(CPack)
