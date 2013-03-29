
add_library(lua MODULE main.c state.c)
target_link_libraries(lua libhaka)

find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
target_link_libraries(lua ${LUA_LIBRARY})

install(TARGETS lua LIBRARY DESTINATION bin/modules)

