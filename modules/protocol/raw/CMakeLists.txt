# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

swig_add_module(raw lua SHARED
	raw.i
	main.c)
SWIG_FIX_ENTRYPOINT(raw protocol)

include_directories(.)

INSTALL_MODULE(raw protocol)

# Tests
add_subdirectory(test)
