
include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(haka-common-ctl STATIC
	ctl_comm.c)
