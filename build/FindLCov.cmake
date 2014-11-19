# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

find_program(LCOV_EXECUTABLE NAMES lcov
	PATH_SUFFIXES bin
	DOC "Lcov coverage tool"
)

find_package_handle_standard_args(LCov REQUIRED_VARS LCOV_EXECUTABLE)
