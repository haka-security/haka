/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/colors.h>
#include <haka/error.h>
#include <haka/log.h>

#include "geoip.h"


GeoIP *geoip_handle = NULL;

INIT static int init(struct parameters *args)
{
	geoip_handle = GeoIP_open("/usr/share/GeoIP/GeoIP.dat", GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE);
	if (!geoip_handle) {
		error(L"cannot initialize geoip");
		return 1;
	}

	GeoIP_set_charset(geoip_handle, GEOIP_CHARSET_UTF8);
	return 0;
}

FINI static void cleanup()
{
	GeoIP_delete(geoip_handle);
	geoip_handle = NULL;
}

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        L"GeoIP lookup utility",
	description: L"Query the geoip database on ip addresses",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
