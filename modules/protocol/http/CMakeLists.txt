
lua_compile(NAME http FILES http.lua)
lua_install(TARGET http DESTINATION ${MODULE_INSTALL_PATH}/protocol)

# Tests
add_subdirectory(test)
