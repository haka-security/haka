# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(haka-common-ctl STATIC
	ctl_comm.c)
