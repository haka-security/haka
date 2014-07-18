# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(ENV{LANG} "C")
set(ENV{LUA_PATH} ${PROJECT_SOURCE_DIR}/src/lua/?.lua)
set(ENV{HAKA_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX})
set(ENV{LD_LIBRARY_PATH} ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/lib)
set(ENV{TZ} Europe/Paris)
set(ENV{INSTALL} install)

execute_process(COMMAND make -C ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/mymodule clean install
	RESULT_VARIABLE HAD_ERROR
)

if(HAD_ERROR)
	message(FATAL_ERROR "mymodule compilation failed")
endif(HAD_ERROR)
