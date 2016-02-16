# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(OpenSSL)

if(OPENSSL_FOUND)
	swig_process(cryptoswig lua crypto.i)

	add_library(crypto SHARED
		main.c
		base64.c
		${SWIG_cryptoswig_FILES}
	)

	include_directories(${OPENSSL_INCLUDE_DIR})
	target_link_libraries(crypto LINK_PRIVATE ${OPENSSL_LIBRARIES})

	SWIG_FIX_ENTRYPOINT(crypto misc)
	INSTALL_MODULE(crypto misc)
else()
    message(STATUS "Not building module crypto (missing libraries)")
endif()
