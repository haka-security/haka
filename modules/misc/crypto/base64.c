/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <haka/error.h>


char *crypto_base64_encode(const char *data, size_t size, size_t *outsize)
{
	BIO *bio=NULL, *b64;
	BUF_MEM *buffer_ptr;

	bio = BIO_new(BIO_s_mem());
	if (!bio) {
		error("memory error");
		goto err;
	}

	b64 = BIO_new(BIO_f_base64());
	if (!b64) {
		error("memory error");
		goto err;
	}

	bio = BIO_push(b64, bio);
	if (!bio) {
		error("openssl error");
		goto err;
	}

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	if (BIO_write(bio, data, size) != size) {
		error("openssl error");
		goto err;
	}

	BIO_flush(bio);

	BIO_get_mem_ptr(bio, &buffer_ptr);
	BIO_set_close(bio, BIO_NOCLOSE);

	BIO_free_all(bio);

	if (outsize) *outsize = buffer_ptr->length;
	return buffer_ptr->data;

err:
	if (bio) BIO_free_all(bio);
	return NULL;
}

static size_t _crypto_base64_calc_length(const char *data, size_t len)
{
	size_t padding = 0;
	assert(len > 2);
	if (data[len-1] == '=') {
		if (data[len-2] == '=') padding = 2;
		else padding = 1;
	}
	return (len*3)/4 - padding;
}

char *crypto_base64_decode(const char *data, size_t size, size_t *outsize)
{
	BIO *bio=NULL, *b64;
	char *result = NULL;
	const size_t len = _crypto_base64_calc_length(data, size);
	int check_len;

	result = malloc(len + 1);
	if (!result) {
		error("memory error");
		goto err;
	}

	bio = BIO_new_mem_buf((char *)data, size);
	if (!bio) {
		error("memory error");
		goto err;
	}

	b64 = BIO_new(BIO_f_base64());
	if (!b64) {
		error("memory error");
		goto err;
	}

	bio = BIO_push(b64, bio);
	if (!bio) {
		error("openssl error");
		goto err;
	}

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	check_len = BIO_read(bio, result, size);
	if (check_len < 0) {
		error("openssl error");
		goto err;
	}

	assert(check_len == len);
	result[len] = '\0';
	if (outsize) *outsize = len;

	BIO_free_all(bio);
	return result;

err:
	if (bio) BIO_free_all(bio);
	free(result);
	return NULL;
}
