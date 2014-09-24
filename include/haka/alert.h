/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Security alerts.
 */

#ifndef _HAKA_ALERT_H
#define _HAKA_ALERT_H

#include <haka/types.h>
#include <haka/time.h>
#include <haka/container/list.h>


/**
 * Alert level.
 */
typedef enum {
	HAKA_ALERT_LEVEL_NONE, /**< Unset value. */
	HAKA_ALERT_LOW,        /**< Low level. */
	HAKA_ALERT_MEDIUM,     /**< Medium level. */
	HAKA_ALERT_HIGH,       /**< High level. */
	HAKA_ALERT_NUMERIC,    /**< Numeric level stored in the alert structure. */

	HAKA_ALERT_LEVEL_LAST  /**< Last alert level. */
} alert_level;

/**
 * Alert completion.
 */
typedef enum {
	HAKA_ALERT_COMPLETION_NONE, /**< Unset value. */
	HAKA_ALERT_FAILED,          /**< The attack failed. */
	HAKA_ALERT_SUCCESSFUL,      /**< The attach was successful. */

	HAKA_ALERT_COMPLETION_LAST  /**< Last alert completion value. */
} alert_completion;

/**
 * Alert node type.
 */
typedef enum {
	HAKA_ALERT_NODE_ADDRESS,    /**< Address node. */
	HAKA_ALERT_NODE_SERVICE,    /**< Service node. */

	HAKA_ALERT_NODE_LAST        /**< Last alert node type. */
} alert_node_type;

/**
 * Alert node.
 */
struct alert_node {
	alert_node_type     type; /**< Alert node type. */
	char              **list; /**< NULL terminated array of strings. */
};

/**
 * Alert.
 */
struct alert {
	struct time         start_time;          /**< Alert time. */
	struct time         end_time;            /**< Alert time. */
	char               *description;         /**< Alert description. */
	alert_level         severity;            /**< Alert severity (HAKA_ALERT_NUMERIC is not a valid value here). */
	alert_level         confidence;          /**< Alert confidence. */
	double              confidence_num;      /**< Alert confidence numeric value if confidence == HAKA_ALERT_NUMERIC. */
	alert_completion    completion;          /**< Alert completion. */
	char               *method_description;  /**< Alert method description. */
	char              **method_ref;          /**< Alert method references (NULL terminated array). */
	struct alert_node **sources;             /**< Alert sources (NULL terminated array of nodes). */
	struct alert_node **targets;             /**< Alert targets (NULL terminated array of nodes). */
	size_t              alert_ref_count;     /**< Reference count. */
	uint64             *alert_ref;           /**< Array of references. */
};

/**
 * Utility macro to create a new alert.
 *
 * \param name Name of the variable that will be created.
 * \param nsrc Number of sources.
 * \param ntgt Number of targets.
 */
#define ALERT(name, nsrc, ntgt) \
	struct alert_node *_sources[nsrc+1] = {0}; \
	struct alert_node *_targets[ntgt+1] = {0}; \
	struct alert name = { \
		sources: nsrc==0 ? NULL : _sources, \
		targets: ntgt==0 ? NULL : _targets,

/**
 * Finish the alert creation.
 */
#define ENDALERT };

/**
 * Create an alert node.
 *
 * \param alert Alert name.
 * \param name ``sources`` or ``target``.
 * \param index Index of the node.
 * \param type Type of node (see :c:type:`alert_node_type`).
 * \param ... List of strings.
 */
#define ALERT_NODE(alert, name, index, type, ...) \
	struct alert_node _node##name##index = { type }; \
	alert.name[index] = &_node##name##index; \
	char *_node##name##index_list[] = { __VA_ARGS__, NULL }; \
	_node##name##index.list = _node##name##index_list

/**
 * Add alert references.
 *
 * \param alert Alert name.
 * \param count Number of references.
 * \param ... List of alert ids.
 */
#define ALERT_REF(alert, count, ...) \
	uint64 _alert_ref[count] = { __VA_ARGS__ }; \
	alert.alert_ref_count = count; \
	alert.alert_ref = _alert_ref

/**
 * Add method references.
 *
 * \param alert Alert name.
 * \param ... List of strings.
 */
#define ALERT_METHOD_REF(alert, ...) \
	char *_method_ref[] = { __VA_ARGS__, NULL }; \
	alert.method_ref = _method_ref

/**
 * Raise a new alert.
 *
 * \returns The alert unique id.
 */
uint64          alert(const struct alert *alert);

/**
 * Update an existing alert.
 */
bool            alert_update(uint64 id, const struct alert *alert);

/**
 * Convert alert level to human readable string.
 */
const char *alert_level_to_str(alert_level level);

/**
 * Convert alert completion to human readable string.
 */
const char *alert_completion_to_str(alert_completion completion);

/**
 * Convert alert node to human readable string.
 */
const char *alert_node_to_str(alert_node_type type);

/**
 * Convert an alert to a string.
 */
const char     *alert_tostring(uint64 id, const struct time *time, const struct alert *alert,
		const char *header, const char *indent, bool color);

/**
 * Enable display of alerts on stdout.
 */
void            enable_stdout_alert(bool enable);

/**
 * Alert listener object.
 */
struct alerter {
	struct list   list;
	void        (*destroy)(struct alerter *state);
	bool        (*alert)(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert);
	bool        (*update)(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert);
	bool          mark_for_remove; /**< \private */
};

/**
 * Add an alert listener.
 */
bool add_alerter(struct alerter *alerter);

/**
 * Remove an alert listener.
 */
bool remove_alerter(struct alerter *alerter);

/**
 * Remove all alert listener.
 */
void remove_all_alerter();

#endif /* _HAKA_ALERT_H */
