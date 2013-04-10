# - find where netfilter-queue and friends are located.
# NETFILTERQUEUE_FOUND - system has dynamic linking interface available
# NETFILTERQUEUE_INCLUDE_DIR - where dlfcn.h is located.
# NETFILTERQUEUE_LIBRARIES - libraries needed to use netfilter-queue

include(CheckFunctionExists)
include(FindPackageHandleStandardArgs)

find_path(NETFILTERQUEUE_INCLUDE_DIR NAMES libnetfilter_queue/libnetfilter_queue.h)
find_library(NETFILTERQUEUE_LIBRARIES NAMES netfilter_queue)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(NETFILTERQUEUE
	REQUIRED_VARS NETFILTERQUEUE_LIBRARIES NETFILTERQUEUE_INCLUDE_DIR
	VERSION_VAR NETFILTERQUEUE_VERSION_STRING)
