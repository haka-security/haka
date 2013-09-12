
#include <haka/log_module.h>

#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <wchar.h>
#include <limits.h>


/*
 * This buffer will be used to store the message to be sent
 * to syslog. It's the concatenation of module + ": " + message
 */
#define MSG_BUF_SIZE          1024

/*
 * We add an extra message to show when a message has been truncated
 */
#define MSG_TRUNCATED         L"..."
#define MSG_BUF_EXTRA_SIZE    (sizeof(MSG_TRUNCATED)/sizeof(wchar_t)-1) * MB_LEN_MAX

#define HAKA_LOG_FACILITY LOG_LOCAL0


static int init(struct parameters *args)
{
	openlog("haka", LOG_CONS | LOG_PID | LOG_NDELAY, HAKA_LOG_FACILITY);
	return 0;
}

static void cleanup()
{
	closelog();
}

static const int syslog_level[HAKA_LOG_LEVEL_LAST] = {
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_DEBUG,
};

static int convert(char *buffer, const size_t len, const wchar_t *message, int *trunc)
{
	mbstate_t state = {0};
	size_t cur = 0;
	char char_buffer[MB_LEN_MAX];

	assert(len > 0);
	assert(buffer);
	assert(message);
	assert(trunc);

	*trunc = 0;

	while (*message) {
		wchar_t c = *message;

		if (c == L'\n' || c == L'\r' || c == L'\t') c = ' ';

		if (c != 0) {
			size_t clen;

			/* Test if the next character might not fit */
			if (len-1-cur < MB_CUR_MAX) {
				clen = wcrtomb(char_buffer, c, &state);
				if (cur+clen > len-1) {
					*trunc = 1;
					break;
				}

				memcpy(buffer, char_buffer, clen);
			}
			else {
				clen = wcrtomb(buffer, c, &state);
				if (clen == (size_t)-1)
					return -1; /* Character could not be converted */
			}

			buffer += clen;
			cur += clen;
		}

		++message;
	}

	/* Add terminating zero */
	assert(cur < len);
	*buffer = 0;
	return cur;
}

static int convert_concat(char *buffer, int *position, size_t len, const wchar_t *message)
{
	int return_value;
	int trunc;

	return_value = convert(buffer+*position, len-*position, message, &trunc);
	if (return_value < 0) {
		return return_value;
	}
	else {
		*position += return_value;
		return trunc ? -2 : 0;
	}
}

static int log_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	int ret;
	int buffer_position = 0;
	char buffer[MSG_BUF_SIZE+MSG_BUF_EXTRA_SIZE];

	/* Convert to multi-bytes and concat module and message */
	if ((ret = convert_concat(buffer, &buffer_position, MSG_BUF_SIZE, module)) < 0 ||
	    (ret = convert_concat(buffer, &buffer_position, MSG_BUF_SIZE, L": ")) < 0 ||
	    (ret = convert_concat(buffer, &buffer_position, MSG_BUF_SIZE, message)) < 0) {
		if (ret == -2) {
			/* The message has been truncated */
			convert_concat(buffer, &buffer_position, MSG_BUF_SIZE+MSG_BUF_EXTRA_SIZE, MSG_TRUNCATED);
		}
		else {
			syslog(LOG_ERR, "Message could not be converted (encoding failed)");
			return 1;
		}
	}

	/* Send log to syslog */
	assert(lvl >= 0 && lvl < HAKA_LOG_LEVEL_LAST);
	syslog(syslog_level[lvl], "%s", buffer);

	return 0;
}


struct log_module HAKA_MODULE = {
	module: {
		type:        MODULE_LOG,
		name:        L"Syslog logger",
		description: L"Logger to syslog",
		author:      L"Arkoon Network Security",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},
	message:         log_message
};

