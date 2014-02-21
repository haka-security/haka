/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_REGEXP_MODULE_H
#define _HAKA_REGEXP_MODULE_H

#include <haka/module.h>
#include <haka/thread.h>
#include <haka/vbuffer.h>

struct regexp;
struct regexp_sink;

#define REGEXP_RESULT_INIT { offset: -1, size: -1 }
#define REGEXP_RESULT_INITIALIZER(result)\
	do {\
		(result)->offset = -1;\
		(result)->size = -1;\
	} while(0)

struct regexp_result {
	size_t offset;
	size_t size;
};

#define REGEXP_VBRESULT_INIT { offset: -1, size: -1 }
#define REGEXP_VBRESULT_INITIALIZER(result)\
	do {\
		(result)->offset = -1;\
		(result)->size = -1;\
	} while(0)

struct regexp_vbresult {
	// TODO use subbuffer when it will be available
	size_t offset;
	size_t size;
};

struct regexp_module {
	struct module module;

	int                    (*match)(const char *pattern, const char *buffer, int len, struct regexp_result *result);
	int                    (*vbmatch)(const char *pattern, struct vbuffer *vbuf, struct regexp_vbresult *result);

	struct regexp         *(*compile)(const char *pattern);
	void                   (*release_regexp)(struct regexp *regexp);
	int                    (*exec)(struct regexp *regexp, const char *buffer, int len, struct regexp_result *result);
	int                    (*vbexec)(struct regexp *regexp, struct vbuffer *vbuf, struct regexp_vbresult *result);

	struct regexp_sink    *(*create_sink)(struct regexp *regexp);
	void                   (*free_regexp_sink)(struct regexp_sink *re_sink);
	int                    (*feed)(struct regexp_sink *re_sink, const char *buffer, int len, bool eof);
	int                    (*vbfeed)(struct regexp_sink *re_sink, struct vbuffer *vbuf, bool eof);
};

struct regexp  {
	const struct regexp_module *module;
	atomic_t ref_count;
};

struct regexp_sink {
	struct regexp *regexp;
	struct regexp_result result;
};

struct regexp_module *regexp_module_load(const char *module_name, struct parameters *args);
void regexp_module_release(struct regexp_module *module);

#endif /* _HAKA_LOG_MODULE_H */
