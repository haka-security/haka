
#include <syslog.h>

#include <haka/log_module.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <wchar.h>

/* This buffer will be used to store the message to be sent
 * to syslog. It's the concatenation of module + ": " + message
 */
#define MSG_BUF_SIZE 1024
#define EXTRA_BYTES 12
#define HAKA_LOG_FACILITY LOG_LOCAL0

static int init(int argc, char *argv[])
{
	openlog("Haka", LOG_CONS | LOG_PID | LOG_NDELAY, HAKA_LOG_FACILITY);
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

static int convert_concat(char *buffer, int *position, const wchar_t *message)
{
	const int return_value = wcstombs(buffer+*position,
					message, MSG_BUF_SIZE-1-*position);
	if ( return_value < 0 ) {
		return -1;
	}
	else {
		*position += return_value;
		return 0;
	}
}

static int log_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	int buffer_position = 0;
	/* Reserve EXTRA_BYTES bytes to write '...' if MSG_BUF_SIZE too small
	 * EXTRA_BYTES bytes in case of each '.' use 4 bytes in a given locale
	 * */
	char buffer[MSG_BUF_SIZE+EXTRA_BYTES];

	/* Convert to UTF-8 and concat module and message for syslog */
	if (convert_concat(buffer, &buffer_position, module) < 0 ||
	    convert_concat(buffer, &buffer_position, L": ") < 0 ||
	    convert_concat(buffer, &buffer_position, message) < 0) {
		syslog(LOG_ERR, "Message could not be converted (encoding failed)");
	}
	else {
		/* If we fill the MSG_BUF_SIZE, we add  '...' to
		 * understand that there is some char missing
		 */
		if (buffer_position == (MSG_BUF_SIZE-1))
		{
			wcstombs(buffer+buffer_position, L"...", EXTRA_BYTES);
		}
		/* Send log to syslog */
		assert(lvl >= 0 && lvl < HAKA_LOG_LEVEL_LAST);
		syslog(syslog_level[lvl], "%s", buffer);
	}

	return 0;
}


struct log_module HAKA_MODULE = {
	module: {
		type:        MODULE_LOG,
		name:        L"Syslog logger",
		description: L"Logger to syslog",
		author:      L"Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	message:         log_message
};

