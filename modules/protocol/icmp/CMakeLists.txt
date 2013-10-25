
lua_compile(NAME icmp FILES icmp.lua)
lua_install(TARGET icmp DESTINATION ${MODULE_INSTALL_PATH}/protocol)

# Tests
#add_subdirectory(test)
