
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
	time_us             start_time;
	time_us             end_time;
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

uint64          alert(struct alert *alert);
bool            alert_update(uint64 id, struct alert *alert);
const wchar_t  *alert_tostring(uint64 id, time_us time, struct alert *alert, const char *header, const char *indent);
void            enable_stdout_alert(bool enable);

struct alerter {
	struct list   list;
	void        (*destroy)(struct alerter *state);
	bool        (*alert)(struct alerter *state, uint64 id, time_us time, struct alert *alert);
	bool        (*update)(struct alerter *state, uint64 id, time_us time, struct alert *alert);
	bool          mark_for_remove;
};

bool add_alerter(struct alerter *alerter);
bool remove_alerter(struct alerter *alerter);
void remove_all_alerter();

#endif /* _HAKA_ALERT_H */
