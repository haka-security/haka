/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_ALERT_H
#define _HAKA_ALERT_H

#include <wchar.h>
#include <haka/types.h>
#include <haka/time.h>
#include <haka/container/list.h>


typedef enum {
	HAKA_ALERT_LEVEL_NONE,
	HAKA_ALERT_LOW,
	HAKA_ALERT_MEDIUM,
	HAKA_ALERT_HIGH,
	HAKA_ALERT_NUMERIC,

	HAKA_ALERT_LEVEL_LAST
} alert_level;

typedef enum {
	HAKA_ALERT_COMPLETION_NONE,
	HAKA_ALERT_FAILED,
	HAKA_ALERT_SUCCESSFUL,

	HAKA_ALERT_COMPLETION_LAST
} alert_completion;

typedef enum {
	HAKA_ALERT_NODE_ADDRESS,
	HAKA_ALERT_NODE_SERVICE,

	HAKA_ALERT_NODE_LAST
} alert_node_type;

struct alert_node {
	alert_node_type     type;
	wchar_t           **list;
};

struct alert {
	struct time         start_time;
	struct time         end_time;
	wchar_t            *description;
	alert_level         severity;
	alert_level         confidence;
	double              confidence_num;
	alert_completion    completion;
	wchar_t            *method_description;
	wchar_t           **method_ref;
	struct alert_node **sources;
	struct alert_node **targets;
	size_t              alert_ref_count;
	uint64             *alert_ref;
};

#define ALERT(name, nsrc, ntgt) \
	struct alert_node *_sources[nsrc+1] = {0}; \
	struct alert_node *_targets[ntgt+1] = {0}; \
	struct alert name = { \
		sources: nsrc==0 ? NULL : _sources, \
		targets: ntgt==0 ? NULL : _targets,

#define ENDALERT };

#define ALERT_NODE(a, name, index, type, ...) \
	struct alert_node _node##name##index = { type }; \
	a.name[index] = &_node##name##index; \
	wchar_t *_node##name##index_list[] = { __VA_ARGS__, NULL }; \
	_node##name##index.list = _node##name##index_list

#define ALERT_REF(a, count, ...) \
	uint64 _alert_ref[count] = { __VA_ARGS__ }; \
	a.alert_ref_count = count; \
	a.alert_ref = _alert_ref

#define ALERT_METHOD_REF(a, ...) \
	wchar_t *_method_ref[] = { __VA_ARGS__, NULL }; \
	a.method_ref = _method_ref

uint64          alert(const struct alert *alert);
bool            alert_update(uint64 id, const struct alert *alert);
const wchar_t  *alert_tostring(uint64 id, const struct time *time, const struct alert *alert, const char *header, const char *indent);
void            enable_stdout_alert(bool enable);

struct alerter {
	struct list   list;
	void        (*destroy)(struct alerter *state);
	bool        (*alert)(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert);
	bool        (*update)(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert);
	bool          mark_for_remove;
};

bool add_alerter(struct alerter *alerter);
bool remove_alerter(struct alerter *alerter);
void remove_all_alerter();

#endif /* _HAKA_ALERT_H */
