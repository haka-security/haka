/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/alert_module.h>
#include <haka/ipv4-addr.h>
#include <haka/elasticsearch.h>
#include <haka/geoip.h>
#include <haka/error.h>
#include <haka/log.h>

#include <assert.h>
#include <stdlib.h>

#include <jansson.h>

#define ALERT_ID_LENGTH 16


const char ELASTICSEARCH_INDEX[] = "ips";

void json_insert_string(json_t *obj, const char *key, const char *string)
{
	json_t *json_str = json_string(string);
	if (!json_str) {
		error("json string creation error");
		return;
	}
	json_object_set_new(obj, key, json_str);
}

void json_insert_double(json_t *obj, const char *key, double val)
{
	json_t *json_val = json_real(val);
	if (!json_val) {
		error("json real creation error");
		return;
	}
	json_object_set_new(obj, key, json_val);
}

void alert_add_geolocalization(json_t *list, char *address, struct geoip_handle *geoip_handler)
{
	if (geoip_handler) {
		ipv4addr addr;
		char country_code[3];

		if (ipv4_addr_from_string_safe(address, &addr) &&
				geoip_lookup_country(geoip_handler, addr, country_code)) {
			json_t *json_country = json_string(country_code);
			if (!json_country) {
				error("json string creation error");
				return;
			}
			json_array_append_new(list, json_country);
		}
	}
}

json_t *json_get_list(json_t *obj, const char *key)
{
	json_t *array = json_object_get(obj, key);
	if (!array) {
		array = json_array();
		if (!array) {
			error("json array creation error");
			return NULL;
		}
		json_object_set(obj, key, array);
	}
	return array;
}

void json_insert_list(json_t *obj, const char *key, char **array)
{
	json_t *nodes = json_array();
	if (!nodes) {
		error("json array creation error");
		return;
	}

	char **iter;
	json_t *json_str;
	for (iter = array; *iter; ++iter) {
		json_str = json_string(*iter);
		if (!json_str) {
			error("json string creation error");
			return;
		}
		json_array_append(nodes, json_str);
	}
	json_object_set_new(obj, key, nodes);
}


void json_insert_address(json_t *obj, char **list, struct geoip_handle *geoip_handler)
{
	json_t *json_str;
	char **iter;

	json_t *address = json_get_list(obj, "address");
	json_t *geo = json_get_list(obj, "geo");

	if (!(address && geo)) return;

	for (iter = list; *iter; ++iter) {
		alert_add_geolocalization(geo, *iter, geoip_handler);
		json_str = json_string(*iter);
		if (!json_str) {
			error("json string creation error");
			return;
		}
		json_array_append_new(address, json_str);
	}
	json_decref(address);
	json_decref(geo);
}

void json_create_mapping(struct elasticsearch_connector* connector, const char *index)
{
	json_t *mapping = json_pack("{s{s{s{s{s{s{ssss}s{ssss}}}}}}}",
		"mappings", "alert", "properties", "method", "properties",
		"ref", "type", "string", "index", "not_analyzed", "description",
		"type", "string", "index", "not_analyzed"
	);
	if (!mapping) {
		error("json mapping creation error");
		return;
	}
	if (!elasticsearch_newindex(connector, index, mapping)) {
		error("elasticsearch index creation error");
		return;
	}
}

json_t *alert_tojson(const struct time *time, const struct alert *alert, struct geoip_handle *geoip_handler)
{
	json_t *ret = json_object();
	if (!ret) {
		error("json object creation error");
		return NULL;
	}

	{
		char timestr[TIME_BUFSIZE];
		time_format(time, "%Y/%m/%d %H:%M:%S", timestr, TIME_BUFSIZE);
		json_insert_string(ret, "time", timestr);
	}

	if (time_isvalid(&alert->start_time)) {
		char timestr[TIME_BUFSIZE];
		time_tostring(&alert->start_time, timestr, TIME_BUFSIZE);
		json_insert_string(ret, "start time", timestr);
	}

	if (time_isvalid(&alert->end_time)) {
		char timestr[TIME_BUFSIZE];
		time_tostring(&alert->end_time, timestr, TIME_BUFSIZE);
		json_insert_string(ret, "end time", timestr);
	}

	if (alert->severity > HAKA_ALERT_LEVEL_NONE && alert->severity < HAKA_ALERT_NUMERIC) {
		json_insert_string(ret, "severity", alert_level_to_str(alert->severity));
	}

	if (alert->confidence > HAKA_ALERT_LEVEL_NONE) {
		if (alert->confidence == HAKA_ALERT_NUMERIC) {
			json_insert_double(ret, "confidence", alert->confidence_num);
		}
		else {
			json_insert_string(ret, "confidence", alert_level_to_str(alert->confidence));
		}
	}

	if (alert->completion > HAKA_ALERT_COMPLETION_NONE) {
		json_insert_string(ret, "completion", alert_completion_to_str(alert->completion));
	}

	if (alert->description)
		json_insert_string(ret, "description", alert->description);

	if (alert->method_description || alert->method_ref) {
		json_t *desc = json_object();
		if (!desc) {
			error("json object creation error");
			free(ret);
			return NULL;
		}

		if (alert->method_description) {
			json_insert_string(desc, "description", alert->method_description);
		}

		if (alert->method_ref) {
			json_insert_list(desc, "ref", alert->method_ref);
		}

		json_object_set_new(ret, "method", desc);
	}

	struct alert_node **iter;

	if (alert->sources) {
		json_t *sources = json_object();
		if (!sources) {
			error("json object creation error");
			free(ret);
			return NULL;
		}
		for (iter = alert->sources; *iter; ++iter) {
			if (strcmp(alert_node_to_str((*iter)->type), "address") == 0) {
				json_insert_address(sources, (*iter)->list, geoip_handler);
			}
			else {
				json_insert_list(sources, "services", (*iter)->list);
			}
		}
		json_object_set_new(ret, "sources", sources);
	}

	if (alert->targets) {
		json_t *targets = json_object();
		if (!targets) {
			error("json object creation error");
			free(ret);
			return NULL;
		}
		for (iter = alert->targets; *iter; ++iter) {
			if (strcmp(alert_node_to_str((*iter)->type), "address") == 0) {
				json_insert_address(targets, (*iter)->list, geoip_handler);
			}
			else {
				json_insert_list(targets, "services", (*iter)->list);
			}
		}
		json_object_set_new(ret, "targets", targets);
	}

	return ret;
}

struct elasticsearch_alerter {
	struct alerter_module             module;
	struct elasticsearch_connector   *connector;
	char                             *server;
	char						     *index;
	struct geoip_handle              *geoip_handler;
	char                              alert_id_prefix[ELASTICSEARCH_ID_LENGTH + 1];
};

bool check_elasticsearch_connection(struct elasticsearch_alerter *alerter)
{
	if (!alerter->connector) {
		alerter->connector = elasticsearch_connector_new(alerter->server);
		if (!alerter->connector) {
			error("enable to connect to elasticsearch server %s", alerter->server);
			return false;
		}
		json_create_mapping(alerter->connector, alerter->index);
	}
	return true;
}


static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

static bool do_alert(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	struct elasticsearch_alerter *alerter = (struct elasticsearch_alerter *)state;
	char elasticsearch_id[ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1];
	snprintf(elasticsearch_id, ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1,
		"%s%llx", alerter->alert_id_prefix, id);

	if (check_elasticsearch_connection(alerter)) {
		json_t *ret = alert_tojson(time, alert, alerter->geoip_handler);
		return ret && elasticsearch_insert(alerter->connector, alerter->index, "alert", elasticsearch_id, ret);
	}
	return false;
}

static bool do_alert_update(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	struct elasticsearch_alerter *alerter = (struct elasticsearch_alerter *)state;
	char elasticsearch_id[ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1];
	snprintf(elasticsearch_id, ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1,
		"%s%llx", alerter->alert_id_prefix, id);

	if (check_elasticsearch_connection(alerter)) {
		json_t *ret = alert_tojson(time, alert, alerter->geoip_handler);
		return ret && elasticsearch_update(alerter->connector, alerter->index, "alert", elasticsearch_id, ret);
	}
	return false;
}

struct alerter_module *init_alerter(struct parameters *args)
{
	struct elasticsearch_alerter *elasticsearch_alerter = malloc(sizeof(struct elasticsearch_alerter));
	if (!elasticsearch_alerter) {
		error("memory error");
		return NULL;
	}

	elasticsearch_alerter->module.alerter.alert = do_alert;
	elasticsearch_alerter->module.alerter.update = do_alert_update;

	const char *server = parameters_get_string(args, "elasticsearch_server", NULL);
	if (!server) {
		error("missing elasticsearch address server");
		return NULL;
	}

	elasticsearch_alerter->server = strdup(server);
	if (!elasticsearch_alerter->server) {
		error("memory error");
		return NULL;
	}

	elasticsearch_alerter->connector = NULL;

	const char *index = parameters_get_string(args, "elasticsearch_index", NULL);
	if (!index) {
		elasticsearch_alerter->index = strdup(ELASTICSEARCH_INDEX);
	}
	else {
		elasticsearch_alerter->index = strdup(index);
	}
	if (!elasticsearch_alerter->index) {
		error("memory error");
		return NULL;
	}
	messagef(HAKA_LOG_DEBUG, "elasticsearch-alert", "using elasticsearch index %s",
		elasticsearch_alerter->index);

	elasticsearch_genid(elasticsearch_alerter->alert_id_prefix);
	messagef(HAKA_LOG_DEBUG, "elasticsearch-alert", "generating global id prefix %s",
		elasticsearch_alerter->alert_id_prefix);

	const char *database = parameters_get_string(args, "geoip_database", NULL);
	if (database) {
		elasticsearch_alerter->geoip_handler = geoip_initialize(database);
	}
	else {
		elasticsearch_alerter->geoip_handler = NULL;
		messagef(HAKA_LOG_WARNING, "geoip", "missing geoip database");
	}

	return &elasticsearch_alerter->module;
}

void cleanup_alerter(struct alerter_module *module)
{
	struct elasticsearch_alerter *alerter = (struct elasticsearch_alerter *)module;
	if (alerter->connector) {
		elasticsearch_connector_close(alerter->connector);
		free(alerter->server);
		free(alerter->index);
	}
	if (alerter->geoip_handler)
		geoip_destroy(alerter->geoip_handler);
	free(alerter);
}

struct alert_module HAKA_MODULE = {
	module: {
		type:        MODULE_ALERT,
		name:        "Elasticsearch alert",
		description: "Alert output to elasticsearch server",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},
	init_alerter:    init_alerter,
	cleanup_alerter: cleanup_alerter
};
