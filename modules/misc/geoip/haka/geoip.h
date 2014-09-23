/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GEOIP_H_
#define _GEOIP_H_

#include <haka/types.h>
#include <haka/ipv4-addr.h>

#include <GeoIP.h>

struct geoip_handle;

struct geoip_handle *geoip_initialize(const char *database);
void                 geoip_destroy(struct geoip_handle *geoip);

bool                 geoip_lookup_country(struct geoip_handle *geoip, ipv4addr addr,
                            char country_code[3]);

#endif /* _GEOIP_H_ */
