
# Setup rpath for modules
set(CMAKE_INSTALL_RPATH "$ORIGIN/modules" "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

# Build main program
add_executable(haka
	main.c
	module.c
	app.c
	lua/state.c)

target_link_libraries(haka libhaka)

find_package(DL REQUIRED)
target_link_libraries(haka ${DL_LIBRARIES})

# SWIG lua bindings for the lib haka
find_package(SWIG REQUIRED)
include(UseSWIG)
swig_add_module(haka-lua lua STATIC
	lua/app.i
	lua/module.i
    lua/packet.i)
#set_target_properties(libhaka-lua PROPERTIES COMPILE_FLAGS -fPIC)
target_link_libraries(haka haka-lua)

find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
target_link_libraries(haka ${LUA_LIBRARY})

install(TARGETS haka RUNTIME DESTINATION bin)

