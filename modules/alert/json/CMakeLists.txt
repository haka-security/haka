# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(alert-json MODULE main.c)
set_target_properties(alert-json PROPERTIES OUTPUT_NAME elasticsearch)

INCLUDE_MODULE(elasticsearch misc)
target_link_libraries(alert-json libelasticsearch)

DEPENDS_MODULE(alert-json geoip misc)
DEPENDS_MODULE(alert-json ipv4 protocol)
INSTALL_MODULE(alert-json alert)
