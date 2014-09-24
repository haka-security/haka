# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(INSTALL_FULLDIR ${CMAKE_INSTALL_PREFIX}/share/haka/lua)
set(LUAJIT_DIR "external/luajit/src")

set(LUAJIT_CFLAGS -fPIC -DLUAJIT_ENABLE_LUA52COMPAT)
set(LUAJIT_CCDEBUG "")
set(LUAJIT_MAKE_OPT CFLAGS="${LUAJIT_CFLAGS}" CCDEBUG="${LUAJIT_CCDEBUG}" PREFIX=${INSTALL_FULLDIR} )

if(NOT ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
	# Force external unwinding
	# Otherwise we get conflicts: when doing a pthread_cancel, the cancel request
	# is caught by luajit on a pcall and the system ends up in corrupted state.
	# WARNING: When enabling this all C code need to be compiled with funwind-tables
	# (or -fexceptions), see luajit/src/lj_err.c for more details.
	set(LUAJIT_CFLAGS ${LUAJIT_CFLAGS} -funwind-tables -DLUAJIT_UNWIND_EXTERNAL)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(LUAJIT_CFLAGS ${LUAJIT_CFLAGS} -O0 -g -DLUA_USE_APICHECK) # -DLUAJIT_USE_GDBJIT -DLUA_USE_ASSERT
	set(LUAJIT_CCDEBUG ${LUAJIT_CCDEBUG} -g)
elseif(CMAKE_BUILD_TYPE STREQUAL Memcheck)
	set(LUAJIT_CFLAGS ${LUAJIT_CFLAGS} -O0 -g -DLUAJIT_USE_VALGRIND -DLUA_USE_APICHECK) # -DLUAJIT_USE_GDBJIT
	set(LUAJIT_CCDEBUG ${LUAJIT_CCDEBUG} -g)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(LUAJIT_CFLAGS ${LUAJIT_CFLAGS} -g)
	set(LUAJIT_CCDEBUG ${LUAJIT_CCDEBUG} -g)
else()
	set(LUAJIT_CFLAGS ${LUAJIT_CFLAGS} -O2)
endif()

execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${LUAJIT_DIR})
execute_process(COMMAND echo ${LUAJIT_MAKE_OPT} OUTPUT_FILE ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.tmp)
execute_process(COMMAND cmp ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.tmp ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.build
	RESULT_VARIABLE FILE_IS_SAME OUTPUT_QUIET ERROR_QUIET)
if(FILE_IS_SAME)
	execute_process(COMMAND cp ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.tmp ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.opt)
endif(FILE_IS_SAME)

add_custom_target(luajit-sync
	COMMAND rsync -rt ${CMAKE_SOURCE_DIR}/${LUAJIT_DIR} ${CMAKE_BINARY_DIR}/external/luajit
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.build
	COMMAND make -C ${CMAKE_BINARY_DIR}/${LUAJIT_DIR} PREFIX=${INSTALL_FULLDIR} clean
	COMMAND cp ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.opt ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.build
	MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.opt
	DEPENDS luajit-sync
)

add_custom_target(luajit
	COMMAND make -C ${CMAKE_BINARY_DIR}/${LUAJIT_DIR} ${LUAJIT_MAKE_OPT} BUILDMODE=static
	COMMAND make -C ${CMAKE_BINARY_DIR}/${LUAJIT_DIR} ${LUAJIT_MAKE_OPT} BUILDMODE=static LDCONFIG='/sbin/ldconfig -n'
		INSTALL_X='install -m 0755 -p' INSTALL_F='install -m 0644 -p' DESTDIR=${CMAKE_BINARY_DIR}/${LUAJIT_DIR} install
	DEPENDS luajit-sync ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/cmake.build
)

set(LUA_DIR ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/${INSTALL_FULLDIR})
set(LUA_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/src)
set(LUA_LIBRARY_DIR ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/${INSTALL_FULLDIR}/lib/)
set(LUA_LIBRARIES ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/src/libluajit.a)

set(LUA_COMPILER ${CMAKE_SOURCE_DIR}/external/luajit/luajitc -p "${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/src/")
set(LUA_FLAGS_NONE "-g")
set(LUA_FLAGS_DEBUG "-g")
set(LUA_FLAGS_MEMCHECK "-g")
set(LUA_FLAGS_RELEASE "-s")
set(LUA_FLAGS_RELWITHDEBINFO "-g")
set(LUA_FLAGS_MINSIZEREL "-s")

install(DIRECTORY ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/${INSTALL_FULLDIR}/share/
	DESTINATION share/haka/lua/share
	PATTERN man* EXCLUDE
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/${LUAJIT_DIR}/src/
	DESTINATION include/haka/lua
	FILES_MATCHING PATTERN *.h
	PATTERN lj_* EXCLUDE
	PATTERN host* EXCLUDE
	PATTERN jit EXCLUDE
)

set(HAKA_LUAJIT 1)
set(HAKA_LUA51 1)
set(LUA_DEPENDENCY luajit)

if(NOT ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
endif()
