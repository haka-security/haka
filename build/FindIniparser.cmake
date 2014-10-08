# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(INIPARSER_INCLUDE_DIR iniparser.h)
find_library(INIPARSER_LIBRARY iniparser)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Iniparser
	REQUIRED_VARS INIPARSER_LIBRARY INIPARSER_INCLUDE_DIR)
