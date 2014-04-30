/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/error.h>
#include <haka/thread.h>
#include <haka/debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <assert.h>


#define HAKA_ERROR_SIZE    2048

struct local_error {
	bool       is_error;
	wchar_t    error_message[HAKA_ERROR_SIZE];
	char       errno_message[HAKA_ERROR_SIZE];
};

static local_storage_t local_error_key;
static bool error_is_valid = false;


static void error_delete(void *value)
{
	free(value);
}

INIT static void _error_init()
{
	UNUSED const bool ret = local_storage_init(&local_error_key, error_delete);
	assert(ret);

	error_is_valid = true;
}

FINI static void _error_fini()
{
	{
		struct local_error *context = (struct local_error *)local_storage_get(&local_error_key);
		if (context) {
			error_delete(context);
		}
	}

	error_is_valid = false;

	{
		UNUSED const bool ret = local_storage_destroy(&local_error_key);
		assert(ret);
	}
}

static struct local_error *error_context()
{
	struct local_error *context = (struct local_error *)local_storage_get(&local_error_key);
	if (!context) {
		context = malloc(sizeof(struct local_error));
		assert(context);

		context->is_error = false;
		local_storage_set(&local_error_key, context);
	}
	return context;
}


void error(const wchar_t *error, ...)
{
	if (error_is_valid) {
		struct local_error *context = error_context();
		if (!context->is_error) {
			va_list ap;
			va_start(ap, error);
			vswprintf(context->error_message, HAKA_ERROR_SIZE, error, ap);
			context->error_message[HAKA_ERROR_SIZE-1] = 0;
			va_end(ap);

			context->is_error = true;

			BREAKPOINT;
		}
	}
}

const char *errno_error(int errno)
{
	if (error_is_valid) {
		struct local_error *context = error_context();
		if (strerror_r(errno, context->errno_message, HAKA_ERROR_SIZE) == 0) {
			context->error_message[HAKA_ERROR_SIZE-1] = 0;
			return context->errno_message;
		}
	}

	return "Unknown error";
}

bool check_error()
{
	if (error_is_valid) {
		struct local_error *context = error_context();
		return context->is_error;
	}
	else {
		return false;
	}
}

const wchar_t *clear_error()
{
	if (error_is_valid) {
		struct local_error *context = error_context();
		if (context->is_error) {
			context->is_error = false;
			return context->error_message;
		}
	}
	return NULL;
}
