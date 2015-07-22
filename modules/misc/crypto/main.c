/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/colors.h>
#include <haka/error.h>
#include <haka/log.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static INIT int init(struct parameters *args)
{
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	return 0;
}

static void cleanup()
{
}

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        "Crypto lookup utility",
	description: "Crypto utilities based on OpenSSL",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
