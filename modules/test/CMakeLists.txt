
add_library(test MODULE main.c)

install(TARGETS test LIBRARY DESTINATION bin/modules)

