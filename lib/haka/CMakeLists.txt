
# Haka library shared between runtime and modules
add_library(libhaka SHARED
	packet.c
    log.c)

set_target_properties(libhaka PROPERTIES OUTPUT_NAME haka)

# Install
install(TARGETS libhaka LIBRARY DESTINATION bin)

