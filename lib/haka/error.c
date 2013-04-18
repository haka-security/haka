
#include <haka/error.h>
#include <stdio.h>
#include <stdarg.h>


#define HAKA_ERROR_SIZE    1024

//TODO: must be moved to tls
static bool haka_error = false;
static wchar_t haka_error_message[HAKA_ERROR_SIZE];

void error(const wchar_t *error, ...)
{
	if (!haka_error) {
		va_list ap;
		va_start(ap, error);
		vswprintf(haka_error_message, HAKA_ERROR_SIZE, error, ap);
		va_end(ap);

		haka_error = true;
	}
}

bool check_error()
{
	return haka_error;
}

const wchar_t *clear_error()
{
	if (haka_error) {
		haka_error = false;
		return haka_error_message;
	}
	return NULL;
}
