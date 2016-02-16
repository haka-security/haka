/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CRYPTO_BASE64_H_
#define CRYPTO_BASE64_H_

#include <haka/types.h>


char *crypto_base64_encode(const char *data, size_t size, size_t *outsize);
char *crypto_base64_decode(const char *data, size_t size, size_t *outsize);

#endif /* CRYPTO_BASE64_H_ */
