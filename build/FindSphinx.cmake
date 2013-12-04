# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

find_program(SPHINX_EXECUTABLE NAMES sphinx-build
	HINTS $ENV{SPHINX_DIR}
	PATH_SUFFIXES bin
	DOC "Sphinx documentation generator"
)

if(SPHINX_EXECUTABLE)
	set(SPHINX_FOUND)
endif(SPHINX_EXECUTABLE)

find_package_handle_standard_args(Sphinx REQUIRED_VARS SPHINX_EXECUTABLE)
