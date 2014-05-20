/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/log.h>
#include <haka/error.h>

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

static bool post(uint64 id, const struct time *time, const struct alert *alert, bool update)
{
	syslog(LOG_NOTICE, "alert: %s%ls", update ? "update " : "", alert_tostring(id, time, alert, "", " ", false));
	return true;
}

static bool do_alert(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	return post(id, time, alert, false);
}

static bool do_alert_update(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	return post(id, time, alert, true);
}

struct alerter_module *init_alerter(struct parameters *args)
{
	static struct alerter_module static_module = {
		alerter: {
			alert: do_alert,
			update: do_alert_update
		}
	};

	return &static_module;
}

void cleanup_alerter(struct alerter_module *alerter)
{
}

struct alert_module HAKA_MODULE = {
	module: {
		type:        MODULE_ALERT,
		name:        L"Syslog alert",
		description: L"Alert output to syslog",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},
	init_alerter:    init_alerter,
	cleanup_alerter: cleanup_alerter
};

