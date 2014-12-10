# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(DL REQUIRED)
find_package(Editline REQUIRED)

include_directories(.)
include_directories(${INIPARSER_INCLUDE_DIR})
include_directories(${EDITLINE_INCLUDE_DIR})

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Haka library shared between runtime and modules

binding_compile(NAME haka LUA lua/lua/haka.lua)
binding_compile(NAME ref LUA lua/lua/ref.lua)
binding_compile(NAME haka_state LUA lua/lua/haka_state.lua)

swig_process(libhakaswig lua
	lua/hakainit.i
	lua/haka.i
	lua/swig.i
	luadebug/luadebug.i)

lua_compile(NAME libhakalua FILES
	lua/lua/color.lua
	lua/lua/class.lua
	lua/lua/events.lua
	lua/lua/context.lua
	lua/lua/parse.lua
	lua/lua/parse_ctx.lua
	lua/lua/parse_result.lua
	lua/lua/grammar.lua
	lua/lua/grammar_dg.lua
	lua/lua/ccomp.lua
	lua/lua/state_machine.lua
	lua/lua/state.lua
	lua/lua/utils.lua
	lua/lua/dissector.lua
	lua/lua/list.lua
	lua/lua/check.lua
	lua/lua/ffibinding.lua
)
lua_install(TARGET libhakalua DESTINATION share/haka/core)

add_library(libhaka SHARED
	packet.c
	log.c
	log_module.c
	alert.c
	alert_module.c
	module.c
	error.c
	debug.c
	thread.c
	parameters.c
	colors.c
	time.c
	timer.c
	state_machine.c
	vbuffer.c
	vbuffer_data.c
	vbuffer_stream.c
	vbuffer_sub_stream.c
	regexp_module.c
	system.c
	engine.c
	container/list.c
	container/list2.c
	container/vector.c
	lua/state.c
	lua/ref.c
	lua/lua.c
	lua/marshal.c
	lua/object.c
	luadebug/interactive.c
	luadebug/complete.c
	luadebug/debugger.c
	luadebug/utils.c
	luadebug/user.c
	luadebug/user_readline.c
	luadebug/user_remote.c
	${SWIG_libhakaswig_FILES}
	${BINDING_haka_FILES}
	${BINDING_ref_FILES}
	${BINDING_haka_state_FILES}
	../../external/lmarshal/lmarshal.c
)

set_target_properties(libhaka PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)

set_target_properties(libhaka PROPERTIES VERSION ${HAKA_VERSION_MAJOR}.${HAKA_VERSION_MINOR}.${HAKA_VERSION_PATCH}
	SOVERSION ${HAKA_VERSION_MAJOR})

target_link_libraries(libhaka LINK_PRIVATE ${DL_LIBRARIES})
target_link_libraries(libhaka LINK_PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(libhaka LINK_PRIVATE ${INIPARSER_LIBRARY})
target_link_libraries(libhaka LINK_PRIVATE rt)
target_link_libraries(libhaka LINK_PRIVATE m)
target_link_libraries(libhaka LINK_PRIVATE ${EDITLINE_LIBRARY})

target_link_libraries(libhaka LINK_PRIVATE  ${LUA_LIBRARIES})
add_dependencies(libhaka ${LUA_DEPENDENCY})
include_directories(${LUA_INCLUDE_DIR})
link_directories(${LUA_LIBRARY_DIR})

set_target_properties(libhaka PROPERTIES OUTPUT_NAME haka)

# Install
install(TARGETS libhaka LIBRARY DESTINATION lib)

add_subdirectory(test)
