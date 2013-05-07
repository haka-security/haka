
#include <haka/error.h>
#include <haka/thread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define HAKA_ERROR_SIZE    1024

struct local_error {
	bool       is_error;
	wchar_t    error_message[HAKA_ERROR_SIZE];
	char       errno_message[HAKA_ERROR_SIZE];
};

static local_storage_t local_error_key;


static void error_delete(void *value)
{
	free(value);
}

INIT static void error_init()
{
	assert(local_storage_init(&local_error_key, error_delete));
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
	struct local_error *context = error_context();
	if (!context->is_error) {
		va_list ap;
		va_start(ap, error);
		vswprintf(context->error_message, HAKA_ERROR_SIZE, error, ap);
		va_end(ap);

		context->is_error = true;
	}
}

const char *errno_error(int err)
{
	struct local_error *context = error_context();
	if (strerror_r(err, context->errno_message, HAKA_ERROR_SIZE))
		return context->errno_message;
	else
		return "Unknown error";
}

bool check_error()
{
	struct local_error *context = error_context();
	return context->is_error;
}

const wchar_t *clear_error()
{
	struct local_error *context = error_context();
	if (context->is_error) {
		context->is_error = false;
		return context->error_message;
	}
	return NULL;
}
