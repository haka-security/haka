/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module elasticsearch

%{
#include "elasticsearch.h"

#include <haka/time.h>

#include <jansson.h>
#include <uuid/uuid.h>

static char base64_encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                       'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                       'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                       'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                       'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                       'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                       'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                       '4', '5', '6', '7', '8', '9', '+', '='};

#define BASE64_ENCODE_LEN(len) ((len * 4) / 3 + 1)

static void base64_encode(const unsigned char *data,
		size_t input_length, char *output)
{
	int j = 0;

	for (; input_length >= 3; data+=3, input_length-=3) {
		const uint32 triple = (data[2] << 0x10) + (data[1] << 0x08) + data[0];

		output[j++] = base64_encoding_table[(triple >> 0 * 6) & 0x3F];
		output[j++] = base64_encoding_table[(triple >> 1 * 6) & 0x3F];
		output[j++] = base64_encoding_table[(triple >> 2 * 6) & 0x3F];
		output[j++] = base64_encoding_table[(triple >> 3 * 6) & 0x3F];
	}

	/* Leftover */
	if (input_length > 0) {
		const uint32 b0 = data[0];
		const uint32 b1 = input_length > 1 ? data[1] : 0;
		const uint32 b2 = input_length > 2 ? data[2] : 0;

		const uint32 triple = (b2 << 0x10) + (b1 << 0x08) + b0;

		output[j++] = base64_encoding_table[(triple >> 0 * 6) & 0x3F];
		output[j++] = base64_encoding_table[(triple >> 1 * 6) & 0x3F];
		if (input_length > 1) {
			output[j++] = base64_encoding_table[(triple >> 2 * 6) & 0x3F];
			if (input_length > 2) {
				output[j++] = base64_encoding_table[(triple >> 3 * 6) & 0x3F];
			}
		}
	}

	output[j] = '\0';
}
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
			if (!index || !data) { error(L"invalid parameter"); return; }

			elasticsearch_newindex($self, index, data);
		}

		void insert(const char *index, const char *type, const char *id, json_t *data) {
			if (!index || !type || !data) { error(L"invalid parameter"); return; }

			elasticsearch_insert($self, index, type, id, data);
		}

		void update(const char *index, const char *type, const char *id, json_t *data) {
			if (!index || !type || !id || !data) { error(L"invalid parameter"); return; }

			elasticsearch_update($self, index, type, id, data);
		}

		void timestamp(struct time *time, char **TEMP_OUTPUT)
		{
			*TEMP_OUTPUT = malloc(20);
			if (!*TEMP_OUTPUT) {
				error(L"memory error");
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
			static const size_t size = BASE64_ENCODE_LEN(16);
			uuid_t uuid;

			*TEMP_OUTPUT = malloc(size+1);
			if (!*TEMP_OUTPUT) {
				error(L"memory error");
				return;
			}

			uuid_generate(uuid);
			base64_encode(uuid, 16, *TEMP_OUTPUT);
		}
	}
};
