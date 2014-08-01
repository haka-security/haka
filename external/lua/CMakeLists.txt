# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(DL REQUIRED)

set(LUA_SRC_DIR src)

set(LUA_CORE_SRC
	${LUA_SRC_DIR}/src/lapi.c
	${LUA_SRC_DIR}/src/lcode.c
	${LUA_SRC_DIR}/src/lctype.c
	${LUA_SRC_DIR}/src/ldebug.c
	${LUA_SRC_DIR}/src/ldo.c
	${LUA_SRC_DIR}/src/ldump.c
	${LUA_SRC_DIR}/src/lfunc.c
	${LUA_SRC_DIR}/src/lgc.c
	${LUA_SRC_DIR}/src/llex.c
	${LUA_SRC_DIR}/src/lmem.c
	${LUA_SRC_DIR}/src/lobject.c
	${LUA_SRC_DIR}/src/lopcodes.c
	${LUA_SRC_DIR}/src/lparser.c
	${LUA_SRC_DIR}/src/lstate.c
	${LUA_SRC_DIR}/src/lstring.c
	${LUA_SRC_DIR}/src/ltable.c
	${LUA_SRC_DIR}/src/ltm.c
	${LUA_SRC_DIR}/src/lundump.c
	${LUA_SRC_DIR}/src/lvm.c
	${LUA_SRC_DIR}/src/lzio.c
)
set(LUA_LIB_SRC
	${LUA_SRC_DIR}/src/lauxlib.c
	${LUA_SRC_DIR}/src/lbaselib.c
	${LUA_SRC_DIR}/src/lbitlib.c
	${LUA_SRC_DIR}/src/lcorolib.c
	${LUA_SRC_DIR}/src/ldblib.c
	${LUA_SRC_DIR}/src/liolib.c
	${LUA_SRC_DIR}/src/lmathlib.c
	${LUA_SRC_DIR}/src/loslib.c
	${LUA_SRC_DIR}/src/lstrlib.c
	${LUA_SRC_DIR}/src/ltablib.c
	${LUA_SRC_DIR}/src/loadlib.c
	${LUA_SRC_DIR}/src/linit.c
)

add_definitions(-DLUA_USE_POSIX=1)
add_definitions(-DLUA_USE_DLOPEN=1)
add_definitions(-DLUA_ROOT="${CMAKE_INSTALL_PREFIX}/share/haka/lua")

add_library(liblua STATIC ${LUA_CORE_SRC} ${LUA_LIB_SRC})
set_target_properties(liblua PROPERTIES OUTPUT_NAME lua)
set_target_properties(liblua PROPERTIES COMPILE_FLAGS -fPIC)

install(FILES
	${LUA_SRC_DIR}/src/lua.h
	${LUA_SRC_DIR}/src/luaconf.h
	${LUA_SRC_DIR}/src/lualib.h
	${LUA_SRC_DIR}/src/lauxlib.h
	DESTINATION include/haka/lua)

add_executable(luac ${LUA_CORE_SRC} ${LUA_LIB_SRC} ${LUA_SRC_DIR}/src/luac.c)
target_link_libraries(luac m ${DL_LIBRARIES})

set(LUA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${LUA_SRC_DIR}/src/ ${CMAKE_CURRENT_BINARY_DIR}/${LUA_SRC_DIR}/src/ PARENT_SCOPE)
set(LUA_LIBRARIES liblua PARENT_SCOPE)

set(LUA_COMPILER $<TARGET_FILE:luac> PARENT_SCOPE)
set(LUA_FLAGS_NONE "" PARENT_SCOPE)
set(LUA_FLAGS_DEBUG "" PARENT_SCOPE)
set(LUA_FLAGS_MEMCHECK "" PARENT_SCOPE)
set(LUA_FLAGS_RELEASE "-s" PARENT_SCOPE)
set(LUA_FLAGS_RELWITHDEBINFO "" PARENT_SCOPE)
set(LUA_FLAGS_MINSIZEREL "-s" PARENT_SCOPE)

set(LUA_DEPENDENCY liblua luac PARENT_SCOPE)
