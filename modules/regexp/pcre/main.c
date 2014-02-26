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

/* We enforce multiline on all API */
#define DEFAULT_COMPILE_OPTIONS PCRE_MULTILINE

/* workspace vector should contain at least 20 elements
 * see pcreapi(3) */
#define WSCOUNT_DEFAULT 20
/* We don't want workspace to grow over 4 KB = 1024 int32 */
#define WSCOUNT_MAX 1024
/* output vector is set to 3 as we want only 1 result
 * see pcreapi(3) */
#define OVECTOR_SIZE 3

#define CHECK_REGEXP_TYPE(re)\
	do {\
		if (re == NULL || re->regexp.module != &HAKA_MODULE) {\
			error(L"Wrong regexp struct passed to PCRE module");\
			goto type_error;\
		}\
	} while(0)

#define CHECK_REGEXP_SINK_TYPE(sink)\
	do {\
		if (sink == NULL || sink->regexp_sink.regexp->module != &HAKA_MODULE) {\
			error(L"Wrong regexp_sink struct passed to PCRE module");\
			goto type_error;\
		}\
	} while(0)

struct regexp_pcre {
	struct regexp regexp;
	pcre *pcre;
	atomic_t wscount_max;
};

struct regexp_sink_pcre {
	struct regexp_sink regexp_sink;
	bool started;
	int match;
	size_t processed_length;
	atomic_t wscount;
	int *workspace;
};

static int  init(struct parameters *args);
static void cleanup();

static int                   match(const char *pattern, const char *buf, int len, struct regexp_result *result);
static int                   vbmatch(const char *pattern, struct vbuffer_sub *vbuf, struct regexp_vbresult *result);

static struct regexp        *compile(const char *pattern);
static void                  release_regexp(struct regexp *re);
static int                   exec(struct regexp *re, const char *buf, int len, struct regexp_result *result);
static int                   vbexec(struct regexp *re, struct vbuffer_sub *vbuf, struct regexp_vbresult *result);

static struct regexp_sink   *create_sink(struct regexp *re);
static void                  free_regexp_sink(struct regexp_sink *sink);
static int                   feed(struct regexp_sink *sink, const char *buf, int len, bool eof);
static int                   vbfeed(struct regexp_sink *sink, struct vbuffer_sub *vbuf, bool eof);

static int                      _exec(struct regexp *re, const char *buf, int len, struct regexp_result *result);
static int                      _partial_exec(struct regexp_sink_pcre *sink, const char *buf, int len, struct regexp_vbresult *vbresult, bool eof);
static struct regexp_sink_pcre *_create_sink(struct regexp *_re);
static void                     _free_regexp_sink(struct regexp_sink_pcre *sink);
static int                      _vbpartial_exec(struct regexp_sink_pcre *sink, struct vbuffer_sub *vbuf, struct regexp_vbresult *result, bool _eof);

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

	create_sink:      create_sink,
	free_regexp_sink: free_regexp_sink,
	feed:             feed,
	vbfeed:           vbfeed,
};

static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

static int match(const char *pattern, const char *buf, int len, struct regexp_result *result)
{
	int ret = -1;
	struct regexp *re;

	assert(pattern);
	assert(buf);

	re = compile(pattern);
	if (re == NULL) return -1;

	ret = exec(re, buf, len, result);

	release_regexp(re);

	return ret;
}

static int vbmatch(const char *pattern, struct vbuffer_sub *vbuf, struct regexp_vbresult *result)
{
	int ret = -1;
	struct regexp *re;

	assert(pattern);
	assert(vbuf);

	re = compile(pattern);
	if (re == NULL) return -1;

	ret = vbexec(re, vbuf, result);

	release_regexp(re);

	return ret;
}

static struct regexp *compile(const char *pattern)
{
	int options = DEFAULT_COMPILE_OPTIONS;
	const char *errorstr;
	int erroffset;
	struct regexp_pcre *re;

	assert(pattern);

	re = malloc(sizeof(struct regexp_pcre));
	if (!re) {
		error(L"memory error");
		return NULL;
	}


	re->regexp.module = &HAKA_MODULE;
	re->pcre = pcre_compile(pattern, options, &errorstr, &erroffset, NULL);
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

static int exec(struct regexp *re, const char *buf, int len, struct regexp_result *result)
{
	assert(re);
	assert(buf);

	return _exec(re, buf, len, result);
}

static int vbexec(struct regexp *re, struct vbuffer_sub *vbuf, struct regexp_vbresult *result)
{
	int ret = 0;
	struct regexp_sink_pcre *sink;

	assert(re);
	assert(vbuf);

	sink = _create_sink(re);

	if (sink == NULL)
		return -1;

	_vbpartial_exec(sink, vbuf, result, true);

	ret = sink->match;

	_free_regexp_sink(sink);

	return ret;
}

static struct regexp_sink *create_sink(struct regexp *re)
{
	assert(re);

	return (struct regexp_sink *)_create_sink(re);
}

static struct regexp_sink_pcre *_create_sink(struct regexp *_re)
{
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	struct regexp_sink_pcre *sink;

	CHECK_REGEXP_TYPE(re);

	sink = malloc(sizeof(struct regexp_sink_pcre));
	if (!sink) {
		error(L"memory error");
		goto error;
	}

	sink->regexp_sink.regexp = _re;
	sink->started = false;
	sink->match = 0;
	sink->processed_length = 0;
	sink->wscount = re->wscount_max;
	sink->workspace = calloc(sink->wscount, sizeof(int));
	if (!sink->workspace) {
		error(L"memory error");
		goto error;
	}

	atomic_inc(&re->regexp.ref_count);

	return sink;

error:
	if (sink != NULL) free(sink->workspace);
	free(sink);
type_error:
	return NULL;
}

static void free_regexp_sink(struct regexp_sink *_sink)
{
	struct regexp_sink_pcre *sink = (struct regexp_sink_pcre *)_sink;

	CHECK_REGEXP_SINK_TYPE(sink);

	_free_regexp_sink(sink);

type_error:
	return;
}

static void _free_regexp_sink(struct regexp_sink_pcre *sink)
{
	assert(sink);

	release_regexp(sink->regexp_sink.regexp);
	free(sink->workspace);
	free(sink);
}

static bool workspace_grow(struct regexp_sink_pcre *sink)
{
	struct regexp_pcre *re;

	assert(sink);

	re = (struct regexp_pcre *)sink->regexp_sink.regexp;

	sink->wscount *= 2;

	messagef(HAKA_LOG_DEBUG, LOG_MODULE, L"growing PCRE workspace to %d int", sink->wscount);

	if (sink->wscount > WSCOUNT_MAX) {
		error(L"PCRE workspace too big, max allowed size is %d int", WSCOUNT_MAX);
		return false;
	}

	/* Here test and assign are not thread safe.
	 * That could override assigned value
	 * but the assigned value will be different only when a thread will have
	 * growth workspace more than once while the other thread will still be
	 * between test and assign.
	 * Even that worst case is not dangerous since the only effect will be
	 * that new regexp_sink will have undersized workspace.
	 */
	if (sink->wscount > re->wscount_max) {
		re->wscount_max = sink->wscount;
	}

	sink->workspace = realloc(sink->workspace, sink->wscount*sizeof(int));
	if (!sink->workspace) {
		error(L"memory error");
		return false;
	}

	return true;
}

static int feed(struct regexp_sink *_sink, const char *buf, int len, bool eof)
{
	struct regexp_sink_pcre *sink = (struct regexp_sink_pcre *)_sink;

	CHECK_REGEXP_SINK_TYPE(sink);
	assert(buf);

	return _partial_exec(sink, buf, len, NULL, eof);

type_error:
	return -1;
}

static int vbfeed(struct regexp_sink *_sink, struct vbuffer_sub *vbuf, bool _eof)
{
	struct regexp_sink_pcre *sink = (struct regexp_sink_pcre *)_sink;

	CHECK_REGEXP_SINK_TYPE(sink);
	assert(vbuf);

	return _vbpartial_exec(sink, vbuf, NULL, _eof);

type_error:
	return -1;
}

static int _exec(struct regexp *_re, const char *buf, int len, struct regexp_result *result)
{
	int ret = -1;
	struct regexp_pcre *re = (struct regexp_pcre *)_re;
	int ovector[OVECTOR_SIZE] = { 0 };

	CHECK_REGEXP_TYPE(re);
	assert(buf);

	ret = pcre_exec(re->pcre, NULL, buf, len, 0, 0, ovector, OVECTOR_SIZE);

	/* Got some match (ret = 0) if we get more than OVECTOR_SIZE */
	if (ret >= 0) {
		if (result != NULL) {
			result->offset = ovector[0];
			result->size = ovector[1] - ovector[0];
		}
		return  1;
	}

	switch (ret) {
		case PCRE_ERROR_NOMATCH:
			return 0;
		default:
			error(L"PCRE internal error %d", ret);
			return -1;
	}

type_error:
	return -1;
}

static int _partial_exec(struct regexp_sink_pcre *sink, const char *buf, int len, struct regexp_vbresult *vbresult, bool eof)
{
	int ret = -1;
	int options;
	int ovector[OVECTOR_SIZE] = { 0 };
	struct regexp_pcre *re;

	assert(sink);
	assert(buf);

	if (sink->workspace == NULL) {
		error(L"Invalid sink. NULL workspace");
		goto error;
	}

	re = (struct regexp_pcre *)sink->regexp_sink.regexp;

	/* If we already match don't bother with regexp again */
	if (sink->match > 0)
		return sink->match;

try_again:
	/* We use PCRE_PARTIAL_SOFT because we are only interested in full match */
	options = PCRE_PARTIAL_SOFT;
	/* Set pcre exec options */
	if (sink->started) options |= PCRE_NOTBOL;
	/* restart dfa only on partial match
	 * see pcreapi(3) */
	if (sink->match == PCRE_ERROR_PARTIAL) options |= PCRE_DFA_RESTART;
	if (!eof) options |= PCRE_NOTEOL;

	if (!sink->started) sink->started = true;

	do {
		/* We run out of space so grow workspace */
		if (ret == PCRE_ERROR_DFA_WSSIZE) {
			if (!workspace_grow(sink)) {
				goto error;
			}
		}

		ret = pcre_dfa_exec(re->pcre, NULL, buf, len, 0, options, ovector,
				OVECTOR_SIZE, sink->workspace, sink->wscount);
	} while(ret == PCRE_ERROR_DFA_WSSIZE);

	if (ret >= 0) {
		/* If no previous partial match
		 * register start of match */
		if (sink->match == 0) {
			if (vbresult) {
				vbresult->offset = sink->processed_length + ovector[0];
			}
			sink->regexp_sink.result.offset = sink->processed_length + ovector[0];
		}
		/* If first time we match
		 * register end of match */
		if (sink->match != 1) {
			if (vbresult) {
				vbresult->size = sink->processed_length + ovector[1] - vbresult->offset;
			}
			sink->regexp_sink.result.size = sink->processed_length + ovector[1] - sink->regexp_sink.result.offset;
		}
		sink->match = 1;
		sink->processed_length += len;
		return sink->match;
	}

	switch (ret) {
		case PCRE_ERROR_PARTIAL:
			/* On first partial match
			 * register start of match */
			if (sink->match == 0) {
				if (vbresult) {
					vbresult->offset = sink->processed_length + ovector[0];
				}
				sink->regexp_sink.result.offset = sink->processed_length + ovector[0];
			}
			sink->match = PCRE_ERROR_PARTIAL;
			sink->processed_length += len;
			return 0;
		case PCRE_ERROR_NOMATCH:
			/* pcre cannot see a new match when it failed a partial
			 * we workaround this by running it again on current
			 * data.
			 * This leave the following case unhandled :
			 * Try to match /aabc|abd/ on "aab", "d"
			 */
			if (sink->match == PCRE_ERROR_PARTIAL) {
				sink->match = 0;
				goto try_again;
			}
			sink->match = 0;
			sink->processed_length += len;
			return 0;
		default:
			sink->match = -1;
			error(L"PCRE internal error %d", ret);
			return sink->match;
	}

error:
	return -1;
}


static int _vbpartial_exec(struct regexp_sink_pcre *sink, struct vbuffer_sub *vbuf, struct regexp_vbresult *result, bool _eof)
{
	int ret = 0;
	size_t len, plen = 0;
	struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
	const uint8 *pptr = NULL;

	assert(sink);
        assert(vbuf);

	/* In order to avoid vbuffer ending with empty chunck,
	 * we keep previous non-empty ptr in pptr and
	 * wait for end or next non-empty ptr to match against it */
	do {
		const uint8 *ptr = vbuffer_mmap(vbuf, &len, false, &iter);
		bool last = (ptr == NULL);
		bool eof = _eof && last;

		/* PCRE partial don't accept empty buffer see pcreapi(3)
		 * Skip empty one except for end of vbuffer where we want to send previous valid */
		if (len == 0 && !last) continue;

		/* We got a new valid pointer, send previous one to pcre */
		if (pptr) {
			/* eof if last is empty */
			ret = _partial_exec(sink, (const char *)pptr, plen, result, eof && len == 0);
			/* if match or something goes wrong avoid parsing more */
			if (ret != 0) break;
		}

		pptr = ptr;
		plen = len;

	} while (pptr != NULL);

	return sink->match;
}
