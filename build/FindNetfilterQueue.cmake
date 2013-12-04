# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Search for netfilter-queue
# NETFILTERQUEUE_FOUND
# NETFILTERQUEUE_INCLUDE_DIR
# NETFILTERQUEUE_LIBRARIES

include(CheckFunctionExists)
include(FindPackageHandleStandardArgs)

find_path(NETFILTERQUEUE_INCLUDE_DIR NAMES libnetfilter_queue/libnetfilter_queue.h)
find_library(NETFILTERQUEUE_LIBRARIES NAMES netfilter_queue)

find_package_handle_standard_args(NETFILTERQUEUE
	REQUIRED_VARS NETFILTERQUEUE_LIBRARIES NETFILTERQUEUE_INCLUDE_DIR)
