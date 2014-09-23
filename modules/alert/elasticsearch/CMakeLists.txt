# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(alert-elasticsearch MODULE main.c)
set_target_properties(alert-elasticsearch PROPERTIES OUTPUT_NAME elasticsearch)

INCLUDE_MODULE(elasticsearch misc)
target_link_libraries(alert-elasticsearch libelasticsearch)

DEPENDS_MODULE(alert-elasticsearch geoip misc)
DEPENDS_MODULE(alert-elasticsearch ipv4 protocol)
INSTALL_MODULE(alert-elasticsearch alert)
