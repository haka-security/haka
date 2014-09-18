/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/colors.h>
#include <haka/error.h>
#include <haka/log.h>

#include <curl/curl.h>


INIT static int init(struct parameters *args)
{
	if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
		error("unable to initialize curl library");
		return -1;
	}

	return 0;
}

FINI static void cleanup()
{
	curl_global_cleanup();
}

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        "ElasticSearch connector",
	description: "Insert and query an eleastic search server",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
