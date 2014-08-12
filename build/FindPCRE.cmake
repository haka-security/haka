# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Try to find the PCRE library (Perl Compatible Regular Expression)

include(FindPackageHandleStandardArgs)
include(CheckCSourceCompiles)

find_path(PCRE_INCLUDE_DIR NAMES pcre.h)
find_library(PCRE_LIBRARY NAMES pcre)

find_package_handle_standard_args(PCRE REQUIRED_VARS PCRE_LIBRARY PCRE_INCLUDE_DIR)
