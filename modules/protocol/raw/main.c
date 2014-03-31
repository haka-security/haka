/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/module.h>


static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        L"Raw",
	description: L"Raw packet protocol",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
