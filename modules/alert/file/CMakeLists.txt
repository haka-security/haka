# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(alert-file MODULE main.c)
set_target_properties(alert-file PROPERTIES OUTPUT_NAME file)
INSTALL_MODULE(alert-file alert)
