# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(FindPackageHandleStandardArgs)

find_program(RSYNC_EXECUTABLE NAMES rsync
	DOC "Rsync file-copying tool"
)

find_package_handle_standard_args(Rsync REQUIRED_VARS RSYNC_EXECUTABLE)
