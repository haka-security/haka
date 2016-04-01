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

/* Limit the length of alert id suffixes */
#define ALERT_ID_LENGTH 16

static REGISTER_LOG_SECTION(elasticsearch);

const char ELASTICSEARCH_INDEX[] = "ips";

static bool json_insert_string(json_t *obj, const char *key, const char *string)
{
	json_t *json_str = json_string(string);
	if (!json_str) {
		error("json string creation error");
		return false;
	}
	if (json_object_set_new(obj, key, json_str) < 0) {
		error("json object insertion error");
		return false;
	}
	return true;
}

static bool json_insert_double(json_t *obj, const char *key, double val)
{
	json_t *json_val = json_real(val);
	if (!json_val) {
		error("json real creation error");
		return false;
	}
	if (json_object_set_new(obj, key, json_val) < 0) {
		error("json object insertion error");
		return false;
	}
	return true;
}

static bool alert_add_geolocalization(json_t *list, char *address, struct geoip_handle *geoip_handler)
{
	if (geoip_handler) {
		char country_code[3];
		ipv4addr addr = ipv4_addr_from_string(address);
		if (addr && geoip_lookup_country(geoip_handler, addr, country_code)) {
			json_t *json_country = json_string(country_code);
			if (!json_country) {
				error("json string creation error");
				return false;
			}
			if (json_array_append_new(list, json_country)) {
				error("json array insertion error");
				return false;
			}
		}
	}
	return true;
}

json_t *json_get_or_create_list(json_t *obj, const char *key)
{
	json_t *array = json_object_get(obj, key);
	if (!array) {
		array = json_array();
		if (!array) {
			error("json array creation error");
			return NULL;
		}
		if (json_object_set(obj, key, array) < 0) {
			error("json object insertion error");
			json_decref(array);
			return NULL;
		}
	}
	else {
		json_incref(array);
	}
	return array;
}

static bool json_insert_list(json_t *obj, const char *key, char **array)
{
	char **iter;
	json_t *json_str;

	json_t *nodes = json_array();
	if (!nodes) {
		error("json array creation error");
		return false;
	}

	for (iter = array; *iter; ++iter) {
		json_str = json_string(*iter);
		if (!json_str) {
			error("json string creation error");
			json_decref(nodes);
			return false;
		}
		if (json_array_append_new(nodes, json_str)) {
			error("json array insertion error");
			return false;
		}
	}

	if (json_object_set_new(obj, key, nodes) < 0) {
		error("json object insertion error");
		return false;
	}

	return true;
}

static bool json_insert_address(json_t *obj, char **list, struct geoip_handle *geoip_handler)
{
	json_t *json_str;
	char **iter;
	json_t *address, *geo;

	address = json_get_or_create_list(obj, "address");
	if (!address) {
		return false;
	}

	geo = json_get_or_create_list(obj, "geo");
	if (!geo) {
		json_decref(address);
		return false;
	}

	for (iter = list; *iter; ++iter) {
		alert_add_geolocalization(geo, *iter, geoip_handler);
		json_str = json_string(*iter);
		if (!json_str) {
			error("json string creation error");
			json_decref(address);
			json_decref(geo);
			return false;
		}
		if (json_array_append_new(address, json_str)) {
			error("json array insertion error");
			json_decref(address);
			json_decref(geo);
			return false;
		}
	}
	return true;
}

static void json_create_mapping(struct elasticsearch_connector *connector, const char *index)
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
		json_decref(mapping);
		return;
	}
	json_decref(mapping);
}

json_t *alert_tojson(const struct time *time, const struct alert *alert, struct geoip_handle *geoip_handler)
{
	struct alert_node **iter;
	json_t *ret = json_object();
	if (!ret) {
		error("json object creation error");
		return NULL;
	}

	{
		char timestr[TIME_BUFSIZE];
		elasticsearch_formattimestamp(time, timestr, TIME_BUFSIZE);
		if (!json_insert_string(ret, "time", timestr)) {
			json_decref(ret);
			return NULL;
		}
	}

	if (time_isvalid(&alert->start_time)) {
		char timestr[TIME_BUFSIZE];
		time_tostring(&alert->start_time, timestr, TIME_BUFSIZE);
		if (!json_insert_string(ret, "start time", timestr)) {
			json_decref(ret);
			return NULL;
		}
	}

	if (time_isvalid(&alert->end_time)) {
		char timestr[TIME_BUFSIZE];
		time_tostring(&alert->end_time, timestr, TIME_BUFSIZE);
		if (!json_insert_string(ret, "end time", timestr)) {
			json_decref(ret);
			return NULL;
		}
	}

	if (alert->severity > HAKA_ALERT_LEVEL_NONE && alert->severity < HAKA_ALERT_NUMERIC) {
		if (!json_insert_string(ret, "severity", alert_level_to_str(alert->severity))) {
			json_decref(ret);
			return NULL;
		}
	}

	if (alert->confidence > HAKA_ALERT_LEVEL_NONE) {
		if (alert->confidence == HAKA_ALERT_NUMERIC) {
			if (!json_insert_double(ret, "confidence", alert->confidence_num)) {
				json_decref(ret);
				return NULL;
			}
		}
		else {
			if (!json_insert_string(ret, "confidence", alert_level_to_str(alert->confidence))) {
				json_decref(ret);
				return NULL;
			}
		}
	}

	if (alert->completion > HAKA_ALERT_COMPLETION_NONE) {
		if (!json_insert_string(ret, "completion", alert_completion_to_str(alert->completion))) {
			json_decref(ret);
			return NULL;
		}
	}

	if (alert->description) {
		if (!json_insert_string(ret, "description", alert->description)) {
			json_decref(ret);
			return NULL;
		}
	}

	if (alert->method_description || alert->method_ref) {
		json_t *desc = json_object();
		if (!desc) {
			error("json object creation error");
			json_decref(ret);
			return NULL;
		}

		if (alert->method_description) {
			if (!json_insert_string(desc, "description", alert->method_description)) {
				json_decref(desc);
				json_decref(ret);
				return NULL;
			}
		}

		if (alert->method_ref) {
			if (!json_insert_list(desc, "ref", alert->method_ref)) {
				json_decref(desc);
				json_decref(ret);
				return NULL;
			}
		}

		if (json_object_set_new(ret, "method", desc) < 0) {
			error("json object insertion error");
			json_decref(ret);
			return NULL;
		}
	}

	if (alert->sources) {
		json_t *sources = json_object();
		if (!sources) {
			error("json object creation error");
			json_decref(ret);
			return NULL;
		}
		for (iter = alert->sources; *iter; ++iter) {
			if (strcmp(alert_node_to_str((*iter)->type), "address") == 0) {
				if (!json_insert_address(sources, (*iter)->list, geoip_handler)) {
					json_decref(sources);
					json_decref(ret);
					return NULL;
				}
			}
			else {
				if (!json_insert_list(sources, "services", (*iter)->list)) {
					json_decref(sources);
					json_decref(ret);
					return NULL;
				}
			}
		}

		if (json_object_set_new(ret, "sources", sources) < 0) {
			error("json object insertion error");
			json_decref(ret);
			return NULL;
		}
	}

	if (alert->targets) {
		json_t *targets = json_object();
		if (!targets) {
			error("json object creation error");
			json_decref(ret);
			return NULL;
		}
		for (iter = alert->targets; *iter; ++iter) {
			if (strcmp(alert_node_to_str((*iter)->type), "address") == 0) {
				if (!json_insert_address(targets, (*iter)->list, geoip_handler)) {
					json_decref(targets);
					json_decref(ret);
					return NULL;
				}
			}
			else {
				if (!json_insert_list(targets, "services", (*iter)->list)) {
					json_decref(targets);
					json_decref(ret);
				}
			}
		}

		if (json_object_set_new(ret, "targets", targets) < 0) {
			error("json object insertion error");
			json_decref(ret);
			return NULL;
		}
	}

	return ret;
}

struct elasticsearch_alerter {
	struct alerter_module             module;
	struct elasticsearch_connector   *connector;
	char                             *server;
	char                             *index;
	struct geoip_handle              *geoip_handler;
	char                              alert_id_prefix[ELASTICSEARCH_ID_LENGTH + 1];
};

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

	json_t *ret;
	char elasticsearch_id[ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1];
	snprintf(elasticsearch_id, ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1,
		"%s%llx", alerter->alert_id_prefix, id);

	ret = alert_tojson(time, alert, alerter->geoip_handler);
	return ret && elasticsearch_insert(alerter->connector, alerter->index, "alert", elasticsearch_id, ret);
}

static bool do_alert_update(struct alerter *state, uint64 id, const struct time *time, const struct alert *alert)
{
	struct elasticsearch_alerter *alerter = (struct elasticsearch_alerter *)state;

	json_t *ret;
	char elasticsearch_id[ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1];
	snprintf(elasticsearch_id, ELASTICSEARCH_ID_LENGTH + ALERT_ID_LENGTH + 1,
		"%s%llx", alerter->alert_id_prefix, id);

	ret = alert_tojson(time, alert, alerter->geoip_handler);
	return ret && elasticsearch_update(alerter->connector, alerter->index, "alert", elasticsearch_id, ret);
}

void cleanup_alerter(struct alerter_module *module)
{
	struct elasticsearch_alerter *alerter = (struct elasticsearch_alerter *)module;
	if (alerter->connector) {
		elasticsearch_connector_close(alerter->connector);
		free(alerter->server);
		free(alerter->index);
	}
	if (alerter->geoip_handler) {
		geoip_destroy(alerter->geoip_handler);
	}
	free(alerter);
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
		free(elasticsearch_alerter);
		return NULL;
	}

	elasticsearch_alerter->server = strdup(server);
	if (!elasticsearch_alerter->server) {
		error("memory error");
		free(elasticsearch_alerter);
		return NULL;
	}

	elasticsearch_alerter->connector = elasticsearch_connector_new(elasticsearch_alerter->server);
	if (!elasticsearch_alerter->connector) {
		error("enable to connect to elasticsearch server %s", elasticsearch_alerter->server);
		cleanup_alerter(&elasticsearch_alerter->module);
		return NULL;
	}

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
	LOG_DEBUG(elasticsearch, "using elasticsearch index %s",
		elasticsearch_alerter->index);

	elasticsearch_genid(elasticsearch_alerter->alert_id_prefix, ELASTICSEARCH_ID_LENGTH);
	LOG_DEBUG(elasticsearch, "generating global id prefix %s",
		elasticsearch_alerter->alert_id_prefix);

	json_create_mapping(elasticsearch_alerter->connector, elasticsearch_alerter->index);

	const char *database = parameters_get_string(args, "geoip_database", NULL);
	if (database) {
		elasticsearch_alerter->geoip_handler = geoip_initialize(database);
	}
	else {
		elasticsearch_alerter->geoip_handler = NULL;
		LOG_WARNING(elasticsearch, "missing geoip database, the ip geographic data will not be collected");
	}

	return  &elasticsearch_alerter->module;
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
