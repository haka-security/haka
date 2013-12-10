/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/log_module.h>

#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <wchar.h>
#include <limits.h>


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

static int logger_message(struct logger *state, log_level lvl, const wchar_t *module, const wchar_t *message)
{
	/* Send log to syslog */
	assert(lvl >= 0 && lvl < HAKA_LOG_LEVEL_LAST);
	syslog(syslog_level[lvl], "%ls: %ls", module, message);
	return 0;
}

struct logger_module *init_logger(struct parameters *args)
{
	static struct logger_module static_module = {
		logger: {
			message: logger_message
		}
	};

	return &static_module;
}

void cleanup_logger(struct logger_module *logger)
{
}

struct log_module HAKA_MODULE = {
	module: {
		type:        MODULE_LOG,
		name:        L"Syslog logger",
		description: L"Logger to syslog",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},
	init_logger:     init_logger,
	cleanup_logger:  cleanup_logger
};

