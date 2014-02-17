/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <pcre.h>
#include <string.h>

#include <haka/error.h>
#include <haka/log.h>
#include <haka/regexp_module.h>
#include <haka/thread.h>

#define LOG_MODULE L"pcre"

// workspace vector should contain at least 20 elements
// see pcreapi(3)
#define WSCOUNT_DEFAULT 20
// We don't want workspace to grow over 4 KB = 1024 int32
#define WS_MAX 1024

#define CHECK_REGEXP_TYPE(re)\
	do {\
		if (re == NULL || re->regexp.module != &HAKA_MODULE) {\
			error(L"Wrong regexp struct passed to PCRE module");\
			goto type_error;\
		}\
	} while(0)

#define CHECK_REGEXP_CTX_TYPE(re_ctx)\
	do {\
		if (re_ctx == NULL || re_ctx->regexp_ctx.regexp->module != &HAKA_MODULE) {\
			error(L"Wrong regexp_ctx struct passed to PCRE module");\
			goto type_error;\
		}\
	} while(0)

struct regexp_pcre {
	struct regexp regexp;
	pcre *pcre;
	atomic_t wscount_max;
};

struct regexp_ctx_pcre {
	struct regexp_ctx regexp_ctx;
	int last_result;
	atomic_t wscount;
	int *workspace;
};

static int  init(struct parameters *args);
static void cleanup();

static int match(const char *pattern, const char *buf, int len);
static int vbmatch(const char *pattern, struct vbuffer *vbuf);

static struct regexp *compile(const char *pattern);
static void           release_regexp(struct regexp *re);
static int            exec(struct regexp *re, const char *buf, int len);
static int            vbexec(struct regexp *re, struct vbuffer *vbuf);

static struct regexp_ctx *get_ctx(struct regexp *re);
static void               free_regexp_ctx(struct regexp_ctx *re_ctx);
static int                feed(struct regexp_ctx *re_ctx, const char *buf, int len);
static int                vbfeed(struct regexp_ctx *re_ctx, struct vbuffer *vbuf);

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

	compile:        compile,
	release_regexp: release_regexp,
	exec:           exec,
	vbexec:         vbexec,

	get_ctx:         get_ctx,
	free_regexp_ctx: free_regexp_ctx,
	feed:            feed,
	vbfeed:          vbfeed,
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

	if (re == NULL) return -1;

	ret = exec(re, buf, len);

	release_regexp(re);

	return ret;
}

static int vbmatch(const char *pattern, struct vbuffer *vbuf)
{
	int ret;
	struct regexp *re;
	struct regexp_ctx *re_ctx;

	re = compile(pattern);
	if (re == NULL) return -1;

	re_ctx = get_ctx(re);
	if (re_ctx == NULL) {
		ret = -1;
		goto out;
	}

	ret = vbfeed(re_ctx, vbuf);

out:
	free_regexp_ctx(re_ctx);
	release_regexp(re);

	return ret;
}

static struct regexp *compile(const char *pattern)
{
	const char *errorstr;
	int erroffset;
	struct regexp_pcre *re = malloc(sizeof(struct regexp_pcre));
	if (!re) {
		error(L"memory error");
		return NULL;
	}


	re->regexp.module = &HAKA_MODULE;
	re->pcre = pcre_compile(pattern, 0, &errorstr, &erroffset, NULL);
	if (re->pcre == NULL) goto error;
	re->regexp.ref_count = 1;
	re->wscount_max = WSCOUNT_DEFAULT;

	return (struct regexp *)re;

error:
	free(re);
	error(L"PCRE compilation failed with error '%s' at offset %d", errorstr, erroffset);
	return NULL;
}

static void release_regexp(struct regexp *_re)
{
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	CHECK_REGEXP_TYPE(re);

	if (atomic_dec(&re->regexp.ref_count) != 0) return;

	pcre_free(re->pcre);
	free(re);

type_error:
	return;
}

static int exec(struct regexp *_re, const char *buf, int len)
{
	int ret;
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	CHECK_REGEXP_TYPE(re);

	ret = pcre_exec(re->pcre, NULL, buf, len, 0, 0, NULL,
			0);

	if (ret == 0) return 1;

	switch (ret) {
		case PCRE_ERROR_NOMATCH:
			return 0;
		default:
			error(L"PCRE internal error %d", ret);
			return ret;
	}

type_error:
	return -1;
}

static int vbexec(struct regexp *re, struct vbuffer *vbuf)
{
	int ret;
	struct regexp_ctx *re_ctx = get_ctx(re);

	ret = vbfeed(re_ctx, vbuf);

	free_regexp_ctx(re_ctx);

	return ret;
}

static struct regexp_ctx *get_ctx(struct regexp *_re)
{
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	struct regexp_ctx_pcre *re_ctx;
	CHECK_REGEXP_TYPE(re);

	re_ctx = malloc(sizeof(struct regexp_ctx_pcre));
	if (!re_ctx) {
		error(L"memory error");
		goto error;
	}

	re_ctx->regexp_ctx.regexp = _re;
	re_ctx->last_result = PCRE_ERROR_NOMATCH;
	re_ctx->wscount = re->wscount_max;
	re_ctx->workspace = calloc(re_ctx->wscount, sizeof(int));
	if (!re_ctx->workspace) {
		error(L"memory error");
		goto error;
	}

	atomic_inc(&re->regexp.ref_count);

	return (struct regexp_ctx *)re_ctx;

error:
	if (re_ctx != NULL) free(re_ctx->workspace);
	free(re_ctx);
type_error:
	return NULL;
}

static void free_regexp_ctx(struct regexp_ctx *_re_ctx)
{
	struct regexp_ctx_pcre *re_ctx = (struct regexp_ctx_pcre *)_re_ctx;
	CHECK_REGEXP_CTX_TYPE(re_ctx);

	release_regexp(re_ctx->regexp_ctx.regexp);
	free(re_ctx->workspace);
	free(re_ctx);

type_error:
	return;
}

static bool workspace_grow(struct regexp_ctx_pcre *re_ctx)
{
	struct regexp_pcre *re = (struct regexp_pcre *)re_ctx->regexp_ctx.regexp;

	re_ctx->wscount *= 2;

	messagef(HAKA_LOG_DEBUG, LOG_MODULE, L"growing PCRE workspace to %d int", re_ctx->wscount);

	if (re_ctx->wscount > WS_MAX) {
		error(L"PCRE workspace too big, max allowed size is %d int", WS_MAX);
		return false;
	}

	/* Here test and assign are not thread safe.
	 * That could override assigned value
	 * but the assigned value will be different only when a thread will have
	 * growth workspace more than once while the other thread will still be
	 * between test and assign.
	 * Even that worst case is not dangerous since the only effect will be
	 * that new regexp_ctx will have undersized workspace.
	 */
	if (re_ctx->wscount > re->wscount_max) {
		re->wscount_max = re_ctx->wscount;
	}

	re_ctx->workspace = realloc(re_ctx->workspace, re_ctx->wscount*sizeof(int));
	if (!re_ctx->workspace) {
		error(L"memory error");
		return false;
	}

	return true;
}

static int feed(struct regexp_ctx *_re_ctx, const char *buf, int len)
{
	/* We use PCRE_PARTIAL_SOFT because we are only interested in full match
	 * We use PCRE_DFA_SHORTEST because we want to stop as soon as possible */
	int options = PCRE_PARTIAL_SOFT | PCRE_DFA_SHORTEST;
	struct regexp_pcre *re;
	struct regexp_ctx_pcre *re_ctx = (struct regexp_ctx_pcre *)_re_ctx;
	CHECK_REGEXP_CTX_TYPE(re_ctx);

	if (re_ctx->workspace == NULL) {
		error(L"Invalid re_ctx. NULL workspace");
		goto error;
	}

	re = (struct regexp_pcre *)_re_ctx->regexp;

	if (re_ctx->last_result == PCRE_ERROR_PARTIAL) {
		options |= PCRE_DFA_RESTART;
	}

	do {
		/* We run out of space so grow workspace */
		if (re_ctx->last_result == PCRE_ERROR_DFA_WSSIZE) {
			if (!workspace_grow(re_ctx)) {
				goto error;
			}
		}

		re_ctx->last_result = pcre_dfa_exec(re->pcre, NULL,
				buf, len, 0, options, NULL, 0,
				re_ctx->workspace, re_ctx->wscount);
	} while(re_ctx->last_result == PCRE_ERROR_DFA_WSSIZE);

	if (re_ctx->last_result == 0) return 1;

	switch (re_ctx->last_result) {
		case PCRE_ERROR_NOMATCH:
		case PCRE_ERROR_PARTIAL:
			return 0;
		default:
			error(L"PCRE internal error %d", re_ctx->last_result);
			return -1;
	}

error:
type_error:
	return -1;
}

static int vbfeed(struct regexp_ctx *re_ctx, struct vbuffer *vbuf)
{
	int ret = 0;
	size_t len;
	void *iter = NULL;
	const uint8 *ptr;

	while ((ptr = vbuffer_mmap(vbuf, &iter, &len, false))) {
		ret = feed(re_ctx, (const char *)ptr, len);
		if (ret != 0) break;
	}

	return ret;
}
