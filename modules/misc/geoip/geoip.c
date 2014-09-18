/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "geoip.h"

#include <assert.h>

#include <haka/error.h>


struct geoip_handle *geoip_initialize(const char *database)
{
	GeoIP *geoip_handle = GeoIP_open(database, GEOIP_MEMORY_CACHE);
	if (!geoip_handle) {
		error("cannot initialize geoip");
		return NULL;
	}

	GeoIP_set_charset(geoip_handle, GEOIP_CHARSET_UTF8);
	return (struct geoip_handle *)geoip_handle;
}

void geoip_destroy(struct geoip_handle *geoip)
{
	GeoIP_delete((GeoIP *)geoip);
}

bool geoip_lookup_country(struct geoip_handle *geoip, ipv4addr addr,
		char country_code[3])
{
	const char *returnedCountry;

	assert(geoip);
	assert(country_code);

	returnedCountry = GeoIP_country_code_by_ipnum((GeoIP *)geoip, addr);
	if (!returnedCountry) return false;

	memcpy(country_code, returnedCountry, 3);
	return true;
}
