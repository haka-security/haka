/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/colors.h>
#include <haka/error.h>
#include <haka/log.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>

enum format {
	PRETTY,
	ONELINE,

	FORMAT_LAST,
};

static const char *format_string[] = {
	"pretty",
	"oneline",
};

struct file_alerter {
	struct alerter_module module;
	FILE *output;
	bool stdio;
	enum format format;
	bool color;
};

static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

static bool write_to_file(struct file_alerter *alerter, uint64 id, const struct time *time, const struct alert *alert, bool update)
{
	char *indent;
	char *color = "", *clear = "";

	switch (alerter->format) {
	case PRETTY:
		indent = "\n\t";
		break;
	case ONELINE: /* Default */
	default:
		indent = " ";
	}

	if (alerter->color) {
		switch (alert->severity) {
		case HAKA_ALERT_LOW:
			color = BOLD CYAN;
			clear = CLEAR;
			break;
		case HAKA_ALERT_MEDIUM:
			color = BOLD YELLOW;
			clear = CLEAR;
			break;
		case HAKA_ALERT_HIGH:
			color = BOLD RED;
			clear = CLEAR;
			break;
		default:
			color = BOLD RED;
			clear = CLEAR;
		}
	}

	flockfile(alerter->output);
	fprintf(alerter->output, "%salert%s: %s%ls\n", color, clear,
            update ? "update " : "", alert_tostring(id, time, alert, "", indent, alerter->color));
	funlockfile(alerter->output);

	return true;
}

static bool do_alert(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	return write_to_file((struct file_alerter *)state, id, time, alert, false);
}

static bool do_alert_update(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	return write_to_file((struct file_alerter *)state, id, time, alert, true);
}

struct alerter_module *init_alerter(struct parameters *args)
{
	struct file_alerter *file_alerter = malloc(sizeof(struct file_alerter));
	if (!file_alerter) {
		message(HAKA_LOG_ERROR, L"file", L"memory error");
		return NULL;
	}

	file_alerter->module.alerter.alert = do_alert;
	file_alerter->module.alerter.update = do_alert_update;

	const char *format = parameters_get_string(args, "format", format_string[PRETTY]);
	int i;
	for (i = 0; i < FORMAT_LAST; i++) {
		if (strcmp(format_string[i], format) == 0) {
			file_alerter->format = i;
			break;
		}
	}

	if (file_alerter->format == FORMAT_LAST) {
		message(HAKA_LOG_ERROR, L"file", L"memory error");
		return NULL;
	}

	const char *filename = parameters_get_string(args, "file", NULL);
	if (filename) {
		message(HAKA_LOG_INFO, L"file", L"openning alert file");
		file_alerter->output = fopen(filename, "a");
		if (!file_alerter->output) {
			message(HAKA_LOG_ERROR, L"file", L"cannot open file for alert");
			return NULL;
		}
	} else {
		file_alerter->stdio = true;
		file_alerter->output = stdout;
	}

	file_alerter->color = colors_supported(fileno(file_alerter->output));

	return &file_alerter->module;
}

void cleanup_alerter(struct alerter_module *module)
{
	struct file_alerter *alerter = (struct file_alerter *)module;

	if (!alerter->stdio) {
		fclose(alerter->output);
	}
}

struct alert_module HAKA_MODULE = {
	module: {
		type:        MODULE_ALERT,
		name:        L"File alert",
		description: L"Alert output to file",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},
	init_alerter:    init_alerter,
	cleanup_alerter: cleanup_alerter
};

