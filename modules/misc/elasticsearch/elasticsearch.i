/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module elasticsearch

%{
#include "haka/elasticsearch.h"

#include <haka/time.h>

#include <jansson.h>
%}

%include "haka/lua/swig.si"
%include "haka/lua/object.si"
%include "json.si"

%nodefaultctor;
%nodefaultdtor;

%rename(connector) elasticsearch_connector;

struct elasticsearch_connector {
	%extend {
		elasticsearch_connector(const char *address) {
			return elasticsearch_connector_new(address);
		}

		~elasticsearch_connector() {
			elasticsearch_connector_close($self);
		}

		void newindex(const char *index, json_t *data) {
			if (!index || !data) { error("invalid parameter"); return; }

			elasticsearch_newindex($self, index, data);
		}

		void insert(const char *index, const char *type, const char *id, json_t *data) {
			if (!index || !type || !data) { error("invalid parameter"); return; }

			elasticsearch_insert($self, index, type, id, data);
		}

		void update(const char *index, const char *type, const char *id, json_t *data) {
			if (!index || !type || !id || !data) { error("invalid parameter"); return; }

			elasticsearch_update($self, index, type, id, data);
		}

		void timestamp(struct time *time, char **TEMP_OUTPUT)
		{
			*TEMP_OUTPUT = malloc(20);
			if (!*TEMP_OUTPUT) {
				error("memory error");
				return;
			}

			if (!time_format(time, "%Y/%m/%d %H:%M:%S", *TEMP_OUTPUT, 20)) {
				assert(check_error());
				free(*TEMP_OUTPUT);
				*TEMP_OUTPUT = NULL;
			}
		}

		void genid(char **TEMP_OUTPUT)
		{
			*TEMP_OUTPUT = malloc(ELASTICSEARCH_ID_LENGTH + 1);
			if (!*TEMP_OUTPUT) {
				error("memory error");
				return;
			}

			elasticsearch_genid(*TEMP_OUTPUT, ELASTICSEARCH_ID_LENGTH);
		}
	}
};
