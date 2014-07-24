/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "geoip.h"

#include <assert.h>


bool geoip_lookup_country(ipv4addr addr, char *country_code)
{
	const char *returnedCountry;

	assert(geoip_handle);
	assert(country_code);

	returnedCountry = GeoIP_country_code_by_ipnum(geoip_handle, addr);
	if (!returnedCountry) return false;

	memcpy(country_code, returnedCountry, 3);
	return true;
}
