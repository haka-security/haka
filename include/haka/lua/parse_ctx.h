/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_PARSE_CTX_H
#define _HAKA_LUA_PARSE_CTX_H

#include <stdlib.h>
#include <string.h>

#define INIT_MARKS_SIZE     20
#define INIT_CATCHES_SIZE   20
#define INIT_VALIDATES_SIZE 20
#define INIT_RETAINS_SIZE   20
#define INIT_RESULTS_SIZE   20

#define RECURS_MAX   200
#define RECURS_NODE  0
#define RECURS_LEVEL 1

struct mark {
};

struct catch {
};

struct validate {
};

struct retain {
};

struct recurs {
	int node;
	int level;
};

struct result {
};

struct parse_ctx {
	bool            run;
	int             node;
	int             compound_level;
	/**
	 * recurs_finish_level is the current compound level when a recursion
	 * is started. The recursion will continue when we get back to this
	 * compound level.
	 */
	int             recurs_finish_level;
	int             recurs_count;
	struct recurs   recurs[RECURS_MAX];
	int             bitoffset;
	struct mark     *marks;
	int             mark_count;
	struct catch    *catches;
	int             catch_count;
	struct validate *validates;
	int             validate_count;
	struct retain   *retains;
	int             retain_count;
	struct result   *results;
	int             result_count;
};

#define PARSE_CTX_INIT { true, 1, 0, 0, 0, { { 0 } }, bitoffset }

inline void parse_ctx_init(struct parse_ctx *ctx)
{
	ctx->run                 = true;
	ctx->node                = 1;
	ctx->compound_level      = 0;
	ctx->recurs_finish_level = 0;
	ctx->recurs_count        = 0;
	ctx->bitoffset           = 0;

	memset(ctx->recurs, 0, RECURS_MAX*sizeof(struct recurs));

	ctx->marks = calloc(INIT_MARKS_SIZE, sizeof(struct mark));
	if (!ctx->marks) error("memory error");
	memset(ctx->marks, 0, INIT_MARKS_SIZE*sizeof(struct mark));
	ctx->mark_count = 0;

	ctx->catches = calloc(INIT_CATCHES_SIZE, sizeof(struct catch));
	if (!ctx->catches) error("memory error");
	memset(ctx->catches, 0, INIT_CATCHES_SIZE*sizeof(struct catch));
	ctx->catch_count = 0;

	ctx->validates = calloc(INIT_VALIDATES_SIZE, sizeof(struct validate));
	if (!ctx->validates) error("memory error");
	memset(ctx->validates, 0, INIT_VALIDATES_SIZE*sizeof(struct validate));
	ctx->validate_count = 0;

	ctx->retains = calloc(INIT_RETAINS_SIZE, sizeof(struct retain));
	if (!ctx->retains) error("memory error");
	memset(ctx->retains, 0, INIT_RETAINS_SIZE*sizeof(struct retain));
	ctx->retain_count = 0;

	ctx->results = calloc(INIT_RESULTS_SIZE, sizeof(struct result));
	if (!ctx->results) error("memory error");
	memset(ctx->results, 0, INIT_RESULTS_SIZE*sizeof(struct result));
	ctx->result_count = 0;
}

inline void parse_ctx_free(struct parse_ctx *ctx)
{
	free(ctx->marks);
	free(ctx->catches);
	free(ctx->validates);
	free(ctx->retains);
	free(ctx->results);
}

#endif /* _HAKA_LUA_PARSE_CTX_H */
