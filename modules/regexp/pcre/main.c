/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <pcre.h>

#include <haka/error.h>
#include <haka/regexp_module.h>

#define OVECSIZE 3
#define WSCOUNT 512

#define CHECK_REGEXP_TYPE(regexp_pcre)\
        do {\
                if (regexp_pcre == NULL || regexp_pcre->regexp.module != &HAKA_MODULE) {\
                        error(L"Wrong regexp struct passed to PCRE module");\
                        goto error;\
                }\
        } while(0);

struct regexp_pcre {
        struct regexp regexp;
	pcre *pcre;
        int last_result;
        int workspace[WSCOUNT];
};

static int init(struct parameters *args);
static void cleanup();
static struct regexp *regexp_compile(const char *pattern);
static void regexp_free(struct regexp *regexp);
static int regexp_exec(const struct regexp *regexp, const char *buffer, int len);
static int regexp_match(const char *regexp, const char *buffer, int len);
static int regexp_feed(const struct regexp *regexp, const char *buffer, int len);

struct regexp_module HAKA_MODULE = {
	module: {
		type:        MODULE_REGEXP,
		name:        L"PCRE regexp engine",
		description: L"PCRE regexp engine",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
	},
	compile: regexp_compile,
	free: regexp_free,
	exec: regexp_exec,
	match: regexp_match,
	feed: regexp_feed,
};

static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

static struct regexp *regexp_compile(const char *pattern)
{
	struct regexp_pcre *regexp_pcre = malloc(sizeof(struct regexp_pcre));
	const char *errorstr;
	int erroffset;

	assert(regexp_pcre);

	regexp_pcre->regexp.module = &HAKA_MODULE;
	regexp_pcre->pcre = pcre_compile(pattern, 0, &errorstr, &erroffset, NULL);
	if (regexp_pcre->pcre == NULL)
		goto error;
        regexp_pcre->last_result = 0;

	return (struct regexp *)regexp_pcre;

error:
	free(regexp_pcre);
	error(L"PCRE compilation failed with error '%s' at offset %d", errorstr, erroffset);
	return NULL;
}

static void regexp_free(struct regexp *regexp)
{
        struct regexp_pcre *regexp_pcre = (struct regexp_pcre *)regexp;
        CHECK_REGEXP_TYPE(regexp_pcre);

	pcre_free(regexp_pcre->pcre);
	free(regexp_pcre);

error:
        return;
}

static int regexp_exec(const struct regexp *regexp, const char *buffer, int len)
{
        int ret;
	int ovector[OVECSIZE];
        struct regexp_pcre *regexp_pcre = (struct regexp_pcre *)regexp;
        CHECK_REGEXP_TYPE(regexp_pcre);

	ret = pcre_exec(regexp_pcre->pcre, NULL, buffer, len, 0, 0, ovector,
                        OVECSIZE);

        if (ret >= 0)
                return ret;

        switch (ret) {
                case PCRE_ERROR_NOMATCH:
                        return 0;
                default:
                        error(L"PCRE internal error %d", ret);
                        return ret;
        }

error:
        return -1;
}

static int regexp_match(const char *pattern, const char *buffer, int len)
{
	int ret = 0;
	struct regexp *regexp = regexp_compile(pattern);
	if (regexp == NULL)
		return 0;

	ret = regexp_exec(regexp, buffer, len);

	regexp_free(regexp);

	return ret;
}

static int regexp_feed(const struct regexp *regexp, const char *buffer, int len)
{
        int ovector[OVECSIZE];
        int options = PCRE_PARTIAL_SOFT;
        struct regexp_pcre *regexp_pcre = (struct regexp_pcre *)regexp;
        CHECK_REGEXP_TYPE(regexp_pcre);

        if (regexp_pcre->last_result == PCRE_ERROR_PARTIAL)
                options |= PCRE_DFA_RESTART;

        regexp_pcre->last_result = pcre_dfa_exec(regexp_pcre->pcre, NULL,
                        buffer, len, 0, options, ovector, OVECSIZE,
                        regexp_pcre->workspace, WSCOUNT);

        if (regexp_pcre->last_result >= 0)
                return regexp_pcre->last_result;

        switch (regexp_pcre->last_result) {
                case PCRE_ERROR_NOMATCH:
                case PCRE_ERROR_PARTIAL:
                        return 0;
                default:
                        error(L"PCRE internal error %d", regexp_pcre->last_result);
                        return regexp_pcre->last_result;
        }

error:
        return -1;
}
