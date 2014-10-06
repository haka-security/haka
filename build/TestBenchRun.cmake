# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(ENV{LANG} "C")
set(ENV{LC_ALL} "C")
set(ENV{BUILD_DIR} ${CTEST_MODULE_DIR})
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)
set(ENV{HAKA} ${HAKA})
set(ENV{HAKA_PATH} ${HAKA_PATH})
set(ENV{PATH} $ENV{PATH}:${HAKA_PATH}/sbin:${HAKA_PATH}/bin)
set(ENV{LD_LIBRARY_PATH} ${HAKA_PATH}/lib:${HAKA_PATH}/lib/haka/modules/protocol:${HAKA_PATH}/lib/haka/modules/packet)
set(ENV{TZ} Europe/Paris)

set(CMAKE_MODULE_PATH ${CTEST_MODULE_DIR} ${CMAKE_MODULE_PATH})

message("Executing TZ=\"Europe/Paris\" LANG=\"C\" LC_ALL=\"C\" LUA_PATH=\"$ENV{LUA_PATH}\" HAKA=\"$ENV{HAKA}\" HAKA_PATH=\"$ENV{HAKA_PATH}\" LD_LIBRARY_PATH=\"$ENV{LD_LIBRARY_PATH}\" ${EXE} ${EXE_OPTIONS} ${BENCH}")

execute_process(COMMAND ${EXE} ${EXE_OPTIONS} ${BENCH} RESULT_VARIABLE HAD_ERROR)

if(HAD_ERROR)
	message(FATAL_ERROR "Benchmark script failed")
endif(HAD_ERROR)
