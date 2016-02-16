/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module crypto

%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%{
#include "haka/base64.h"

static void _crypto_base64_encode_wrapper(const char *STRING, size_t SIZE, char **TEMP_OUTPUT, size_t *TEMP_SIZE)
{
	*TEMP_OUTPUT = crypto_base64_encode(STRING, SIZE, TEMP_SIZE);
}

static void _crypto_base64_decode_wrapper(const char *STRING, size_t SIZE, char **TEMP_OUTPUT, size_t *TEMP_SIZE)
{
	*TEMP_OUTPUT = crypto_base64_decode(STRING, SIZE, TEMP_SIZE);
}
%}

%nodefaultctor;
%nodefaultdtor;

void _crypto_base64_encode_wrapper(const char *STRING, size_t SIZE, char **TEMP_OUTPUT, size_t *TEMP_SIZE);
void _crypto_base64_decode_wrapper(const char *STRING, size_t SIZE, char **TEMP_OUTPUT, size_t *TEMP_SIZE);

%luacode{
	local this = unpack({...})

	this.base64 = {}
	this.base64.encode = this._crypto_base64_encode_wrapper
	this.base64.decode = this._crypto_base64_decode_wrapper

	this._crypto_base64_decode_wrapper = nil
	this._crypto_base64_encode_wrapper = nil
}
