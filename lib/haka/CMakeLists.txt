
# Haka library shared between runtime and modules
add_library(libhaka SHARED
	packet.c
	log.c
	module.c
	error.c)

find_package(DL REQUIRED)
target_link_libraries(libhaka ${DL_LIBRARIES})

set_target_properties(libhaka PROPERTIES OUTPUT_NAME haka)

# Install
install(TARGETS libhaka LIBRARY DESTINATION bin)

