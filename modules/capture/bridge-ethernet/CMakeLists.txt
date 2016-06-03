# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(capture-bridge-ethernet SHARED main.c)
set_target_properties(capture-bridge-ethernet PROPERTIES OUTPUT_NAME bridge-ethernet)

INSTALL_MODULE(capture-bridge-ethernet capture)

# Tests
add_subdirectory(test)
