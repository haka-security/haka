# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(log-syslog MODULE main.c)
set_target_properties(log-syslog PROPERTIES OUTPUT_NAME syslog)
INSTALL_MODULE(log-syslog log)
