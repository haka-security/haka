
add_library(test MODULE main.c)
target_link_libraries(test libhaka)

install(TARGETS test LIBRARY DESTINATION bin/modules)

