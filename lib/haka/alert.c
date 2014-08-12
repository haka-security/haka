/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <haka/alert.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/alert_module.h>
#include <haka/container/list.h>
#include <haka/colors.h>


static struct alerter *alerters = NULL;
static local_storage_t alert_string_key;
static atomic64_t alert_id;
static rwlock_t alert_module_lock = RWLOCK_INIT;

#define BUFFER_SIZE    2048

static void alert_string_delete(void *value)
{
	free(value);
}

INIT static void _alert_init()
{
	UNUSED bool ret;

	ret = local_storage_init(&alert_string_key, alert_string_delete);
	assert(ret);

	atomic64_init(&alert_id, 0);
}

FINI static void _alert_fini()
{
	remove_all_alerter();
	atomic64_destroy(&alert_id);

	{
		wchar_t *buffer = local_storage_get(&alert_string_key);
		if (buffer) {
			alert_string_delete(buffer);
		}
	}
}

bool add_alerter(struct alerter *alerter)
{
	assert(alerter);

	rwlock_writelock(&alert_module_lock);
	list_insert_before(alerter, alerters, &alerters, NULL);
	rwlock_unlock(&alert_module_lock);

	return true;
}

static void cleanup_alerter(struct alerter *alerter, struct alerter **head)
{
	rwlock_writelock(&alert_module_lock);
	list_remove(alerter, head, NULL);
	rwlock_unlock(&alert_module_lock);

	alerter->destroy(alerter);
}

bool remove_alerter(struct alerter *alerter)
{
	struct alerter *module_to_release = NULL;
	struct alerter *iter;

	assert(alerter);

	{
		rwlock_readlock(&alert_module_lock);

		for (iter=alerters; iter; iter = list_next(iter)) {
			if (iter == alerter) {
				module_to_release = iter;
				break;
			}
		}

		rwlock_unlock(&alert_module_lock);
	}

	if (!iter) {
		error(L"Alert module is not registered");
		return false;
	}

	if (module_to_release) {
		cleanup_alerter(alerter, &alerters);
	}

	return true;
}

void remove_all_alerter()
{
	struct alerter *alerter = NULL;

	{
		rwlock_writelock(&alert_module_lock);
		alerter = alerters;
		alerters = NULL;
		rwlock_unlock(&alert_module_lock);
	}


	while (alerter) {
		struct alerter *current = alerter;
		alerter = list_next(alerter);

		cleanup_alerter(current, NULL);
	}
}

static void alert_remove_pass()
{
	struct alerter *iter;

	rwlock_readlock(&alert_module_lock);
	for (iter=alerters; iter; iter = list_next(iter)) {
		if (iter->mark_for_remove) {
			rwlock_unlock(&alert_module_lock);
			remove_alerter(iter);
			rwlock_readlock(&alert_module_lock);
		}
	}
	rwlock_unlock(&alert_module_lock);
}

uint64 alert(const struct alert *alert)
{
	const uint64 id = atomic64_inc(&alert_id);
	struct alerter *iter;
	bool remove_pass = false;
	struct time time;

	time_gettimestamp(&time);

	rwlock_readlock(&alert_module_lock);
	for (iter=alerters; iter; iter = list_next(iter)) {
		iter->alert(iter, id, &time, alert);
		remove_pass |= iter->mark_for_remove;
	}
	rwlock_unlock(&alert_module_lock);

	if (remove_pass) alert_remove_pass();

	return id;
}

bool alert_update(uint64 id, const struct alert *alert)
{
	bool remove_pass = false;
	struct alerter *iter;
	struct time time;

	time_gettimestamp(&time);

	rwlock_readlock(&alert_module_lock);
	for (iter=alerters; iter; iter = list_next(iter)) {
		iter->update(iter, id, &time, alert);
		remove_pass |= iter->mark_for_remove;
	}
	rwlock_unlock(&alert_module_lock);

	if (remove_pass) alert_remove_pass();

	return id;
}

static wchar_t *alert_string_context()
{
	wchar_t *context = (wchar_t *)local_storage_get(&alert_string_key);
	if (!context) {
		context = malloc(sizeof(wchar_t)*BUFFER_SIZE);
		assert(context);

		local_storage_set(&alert_string_key, context);
	}
	return context;
}

static const char *str_alert_level[HAKA_ALERT_LEVEL_LAST] = {
	"",
	"low",
	"medium",
	"high",
	"",
};

static const char *alert_level_to_str(alert_level level)
{
	assert(level >= 0 && level < HAKA_ALERT_LEVEL_LAST);
	return str_alert_level[level];
}

static const char *str_alert_completion[HAKA_ALERT_COMPLETION_LAST] = {
	"",
	"failed",
	"successful",
};

static const char *alert_completion_to_str(alert_completion completion)
{
	assert(completion >= 0 && completion < HAKA_ALERT_COMPLETION_LAST);
	return str_alert_completion[completion];
}

static const char *str_alert_node_type[HAKA_ALERT_NODE_LAST] = {
	"address",
	"service",
};

static const char *alert_node_to_str(alert_node_type type)
{
	assert(type >= 0 && type < HAKA_ALERT_NODE_LAST);
	return str_alert_node_type[type];
}

static void alert_string_append(wchar_t **buffer, size_t *len, wchar_t *format, ...)
{
	int count;
	va_list ap;
	va_start(ap, format);
	count = vswprintf(*buffer, *len, format, ap);
	*buffer += count;
	*len -= count;
	va_end(ap);
}

static void alert_stringlist_append(wchar_t **buffer, size_t *len, wchar_t **array)
{
	if (array) {
		wchar_t **iter;
		for (iter = array; *iter; ++iter) {
			if (iter != array)
				alert_string_append(buffer, len, L",");
			alert_string_append(buffer, len, L" %ls", *iter);
		}
	}
}

static void alert_array_append(wchar_t **buffer, size_t *len, wchar_t **array)
{
	alert_string_append(buffer, len, L"{");
	alert_stringlist_append(buffer, len, array);
	alert_string_append(buffer, len, L" }");
}

static void alert_nodes_append(wchar_t **buffer, size_t *len, struct alert_node **array, const char *indent, char *color, char *clear)
{
	struct alert_node **iter;
	alert_string_append(buffer, len, L"{");
	for (iter = array; *iter; ++iter) {
		alert_string_append(buffer, len, L"%s\t%s%s%s:", indent, color, alert_node_to_str((*iter)->type), clear);
		alert_stringlist_append(buffer, len, (*iter)->list);
	}
	alert_string_append(buffer, len, L"%s}", indent);
}

const wchar_t *alert_tostring(uint64 id, const struct time *time, const struct alert *alert, const char *header, const char *indent, bool colored)
{
	wchar_t *buffer = alert_string_context();
	wchar_t *iter = buffer;
	size_t len = BUFFER_SIZE;
	char *color = "", *clear = "";

	if (colored) {
		color = BOLD;
		clear = CLEAR;
	}

	alert_string_append(&iter, &len, L"%s%sid%s = %llu", header, color, clear, id);

	{
		char timestr[TIME_BUFSIZE];
		time_tostring(time, timestr, TIME_BUFSIZE);
		alert_string_append(&iter, &len, L"%s%stime%s = %s", indent, color, clear, timestr);
	}

	if (time_isvalid(&alert->start_time)) {
		char timestr[TIME_BUFSIZE];
		time_tostring(&alert->start_time, timestr, TIME_BUFSIZE);
		alert_string_append(&iter, &len, L"%s%sstart time%s = %s", indent, color, clear, timestr);
	}

	if (time_isvalid(&alert->end_time)) {
		char timestr[TIME_BUFSIZE];
		time_tostring(&alert->end_time, timestr, TIME_BUFSIZE);
		alert_string_append(&iter, &len, L"%s%send time%s = %s", indent, color, clear, timestr);
	}

	if (alert->severity > HAKA_ALERT_LEVEL_NONE && alert->severity < HAKA_ALERT_NUMERIC) {
		alert_string_append(&iter, &len, L"%s%sseverity%s = %s", indent, color, clear,
				alert_level_to_str(alert->severity));
	}

	if (alert->confidence > HAKA_ALERT_LEVEL_NONE) {
		if (alert->confidence == HAKA_ALERT_NUMERIC) {
			alert_string_append(&iter, &len, L"%s%sconfidence%s = %g", indent, color, clear,
					alert->confidence_num);
		}
		else {
			alert_string_append(&iter, &len, L"%s%sconfidence%s = %s", indent, color, clear,
					alert_level_to_str(alert->confidence));
		}
	}

	if (alert->completion > HAKA_ALERT_COMPLETION_NONE) {
		alert_string_append(&iter, &len, L"%s%scompletion%s = %s", indent, color, clear,
				alert_completion_to_str(alert->completion));
	}

	if (alert->description)
		alert_string_append(&iter, &len, L"%s%sdescription%s = %ls", indent, color, clear, alert->description);

	if (alert->method_description || alert->method_ref) {
		alert_string_append(&iter, &len, L"%s%smethod%s = {", indent, color, clear);

		if (alert->method_description) {
			alert_string_append(&iter, &len, L"%s%s\tdescription%s = %ls", indent, color, clear,
					alert->method_description);
		}

		if (alert->method_ref) {
			alert_string_append(&iter, &len, L"%s%s\tref%s = ", indent, color, clear);
			alert_array_append(&iter, &len, alert->method_ref);
		}

		alert_string_append(&iter, &len, L"%s}", indent);
	}

	if (alert->sources) {
		alert_string_append(&iter, &len, L"%s%ssources%s = ", indent, color, clear);
		alert_nodes_append(&iter, &len, alert->sources, indent, color, clear);
	}

	if (alert->targets) {
		alert_string_append(&iter, &len, L"%s%stargets%s = ", indent, color, clear);
		alert_nodes_append(&iter, &len, alert->targets, indent, color, clear);
	}

	if (alert->alert_ref_count && alert->alert_ref) {
		int i;
		alert_string_append(&iter, &len, L"%s%srefs%s = {", indent, color, clear);
		for (i=0; i<alert->alert_ref_count; ++i) {
			if (i != 0)
				alert_string_append(&iter, &len, L",");
			alert_string_append(&iter, &len, L" %llu", alert->alert_ref[i]);
		}
		alert_string_append(&iter, &len, L" }");
	}


	return buffer;
}
