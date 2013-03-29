
add_library(libhaka SHARED
	packet.c)

set_target_properties(libhaka PROPERTIES OUTPUT_NAME haka)

install(TARGETS libhaka LIBRARY DESTINATION bin)

