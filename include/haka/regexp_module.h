/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_REGEXP_MODULE_H
#define _HAKA_REGEXP_MODULE_H

#include <haka/module.h>
#include <haka/vbuffer.h>

struct regexp;
struct regexp_ctx;

struct regexp_module {
	struct module module;

	int                    (*match)(const char *pattern, const char *buffer, int len);
	int                    (*vbmatch)(const char *pattern, struct vbuffer *vbuf);

	struct regexp *        (*compile)(const char *pattern);
	void                   (*free_regexp)(struct regexp *regexp);
	int                    (*exec)(const struct regexp *regexp, const char *buffer, int len);
	int                    (*vbexec)(const struct regexp *regexp, struct vbuffer *vbuf);

	struct regexp_ctx * (*get_ctx)(const struct regexp *regexp);
	void                   (*free_regexp_ctx)(const struct regexp_ctx *re_ctx);
	int                    (*feed)(const struct regexp_ctx *re_ctx, const char *buffer, int len);
	int                    (*vbfeed)(const struct regexp_ctx *re_ctx, struct vbuffer *vbuf);
};

struct regexp  {
	const struct regexp_module *module;
};

struct regexp_ctx {
	const struct regexp *regexp;
};

struct regexp_module *regexp_module_load(const char *module_name, struct parameters *args);

#endif /* _HAKA_LOG_MODULE_H */
