/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <pcre.h>

#include <haka/error.h>
#include <haka/regexp_module.h>

#define OVECSIZE 0
#define WSCOUNT 20

#define CHECK_REGEXP_TYPE(re)\
	do {\
		if (re == NULL || re->regexp.module != &HAKA_MODULE) {\
			error(L"Wrong regexp struct passed to PCRE module");\
			goto error;\
		}\
	} while(0);

#define CHECK_REGEXP_STREAM_TYPE(re_ctx)\
	do {\
		if (re_ctx == NULL || re_ctx->regexp_ctx.regexp->module != &HAKA_MODULE) {\
			error(L"Wrong regexp_ctx struct passed to PCRE module");\
			goto error;\
		}\
	} while(0);

struct regexp_pcre {
	struct regexp regexp;
	pcre *pcre;
};

struct regexp_ctx_pcre {
	struct regexp_ctx regexp_ctx;
	int last_result;
	int workspace[WSCOUNT];
};

static int  init(struct parameters *args);
static void cleanup();

static int match(const char *pattern, const char *buf, int len);
static int vbmatch(const char *pattern, struct vbuffer *vbuf);

static struct regexp *compile(const char *pattern);
static void           free_regexp(struct regexp *re);
static int            exec(const struct regexp *re, const char *buf, int len);
static int            vbexec(const struct regexp *re, struct vbuffer *vbuf);

static struct regexp_ctx *get_ctx(const struct regexp *re);
static void                  free_regexp_ctx(const struct regexp_ctx *re_ctx);
static int                   feed(const struct regexp_ctx *re_ctx, const char *buf, int len);
static int                   vbfeed(const struct regexp_ctx *re_ctx, struct vbuffer *vbuf);

struct regexp_module HAKA_MODULE = {
	module: {
		type:        MODULE_REGEXP,
		name:        L"PCRE regexp engine",
		description: L"PCRE regexp engine",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},

	match:   match,
	vbmatch: vbmatch,

	compile:     compile,
	free_regexp: free_regexp,
	exec:        exec,
	vbexec:      vbexec,

	get_ctx:         get_ctx,
	free_regexp_ctx: free_regexp_ctx,
	feed:               feed,
	vbfeed:             vbfeed,
};

static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

static int match(const char *pattern, const char *buf, int len)
{
	int ret;
	struct regexp *re = compile(pattern);

	if (re == NULL)
		return -1;

	ret = exec(re, buf, len);

	free_regexp(re);

	return ret;
}

static int vbmatch(const char *pattern, struct vbuffer *vbuf)
{
	int ret;
	struct regexp *re;
	struct regexp_ctx *re_ctx;

	re = compile(pattern);
	if (re == NULL)
		return -1;

	re_ctx = get_ctx(re);
	if (re_ctx == NULL)
		return -1;

	ret = vbfeed(re_ctx, vbuf);

	free_regexp_ctx(re_ctx);
	free_regexp(re);

	return ret;
}

static struct regexp *compile(const char *pattern)
{
	struct regexp_pcre *re = malloc(sizeof(struct regexp_pcre));
	const char *errorstr;
	int erroffset;

	assert(re);

	re->regexp.module = &HAKA_MODULE;
	re->pcre = pcre_compile(pattern, 0, &errorstr, &erroffset, NULL);
	if (re->pcre == NULL)
		goto error;

	return (struct regexp *)re;

error:
	free(re);
	error(L"PCRE compilation failed with error '%s' at offset %d", errorstr, erroffset);
	return NULL;
}

static void free_regexp(struct regexp *_re)
{
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	CHECK_REGEXP_TYPE(re);

	pcre_free(re->pcre);
	free(re);

error:
	return;
}

static int exec(const struct regexp *_re, const char *buf, int len)
{
	int ret;
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	CHECK_REGEXP_TYPE(re);

	ret = pcre_exec(re->pcre, NULL, buf, len, 0, 0, NULL,
			OVECSIZE);

	if (ret == 0)
		return 1;

	switch (ret) {
		case PCRE_ERROR_NOMATCH:
			return 0;
		default:
			error(L"PCRE internal error %d", ret);
			return ret;
	}

error:
	return -1;
}

static int vbexec(const struct regexp *re, struct vbuffer *vbuf)
{
	int ret;
	struct regexp_ctx *re_ctx = get_ctx(re);

	ret = vbfeed(re_ctx, vbuf);

	free_regexp_ctx(re_ctx);

	return ret;
}

static struct regexp_ctx *get_ctx(const struct regexp *re)
{
	struct regexp_ctx_pcre *re_ctx = malloc(sizeof(struct regexp_ctx_pcre));
	assert(re_ctx);

	re_ctx->regexp_ctx.regexp = re;
	re_ctx->last_result = PCRE_ERROR_NOMATCH;
	// TODO vector int workspace[WSCOUNT];

	return (struct regexp_ctx *)re_ctx;
}

static void free_regexp_ctx(const struct regexp_ctx *_res)
{
	struct regexp_ctx_pcre *re_ctx = (struct regexp_ctx_pcre *)_res;
	CHECK_REGEXP_STREAM_TYPE(re_ctx);

	// TODO free vector workspace
	free(re_ctx);

error:
	return;
}

static int feed(const struct regexp_ctx *_res, const char *buf, int len)
{
	int options = PCRE_PARTIAL_SOFT | PCRE_DFA_SHORTEST;
	struct regexp_pcre *re;
	struct regexp_ctx_pcre *re_ctx = (struct regexp_ctx_pcre *)_res;
	CHECK_REGEXP_STREAM_TYPE(re_ctx);

	re = (struct regexp_pcre *)_res->regexp;

	if (re_ctx->last_result == PCRE_ERROR_PARTIAL)
		options |= PCRE_DFA_RESTART;

	re_ctx->last_result = pcre_dfa_exec(re->pcre, NULL,
			buf, len, 0, options, NULL, OVECSIZE,
			re_ctx->workspace, WSCOUNT);

	if (re_ctx->last_result == 0)
		return 1;

	switch (re_ctx->last_result) {
		case PCRE_ERROR_NOMATCH:
		case PCRE_ERROR_PARTIAL:
			return 0;
		default:
			error(L"PCRE internal error %d", re_ctx->last_result);
			return re_ctx->last_result;
	}

error:
	return -1;
}

static int vbfeed(const struct regexp_ctx *re_ctx, struct vbuffer *vbuf)
{
	int ret;
	size_t len;
	void *iter = NULL;

	do {
		const uint8 *ptr = vbuffer_mmap(vbuf, &iter, &len, false);
		ret = feed(re_ctx, (const char *)ptr, len);
	} while (ret == 0 && (vbuf = vbuf->next) != NULL);

	return ret;
}
