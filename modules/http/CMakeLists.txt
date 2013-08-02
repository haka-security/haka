
lua_install(NAME http FILES http.lua DESTINATION ${MODULE_INSTALL_PATH} COMPILED)

# Tests
add_subdirectory(test)
