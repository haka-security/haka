/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_PARSE_CTX_H
#define _HAKA_LUA_PARSE_CTX_H

#include <stdlib.h>
#include <string.h>

#include <haka/types.h>

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
	int             bitoffset;
	int             compound_level;
	/**
	 * recurs_finish_level is the current compound level when a recursion
	 * is started. The recursion will continue when we get back to this
	 * compound level.
	 */
	int             recurs_finish_level;
	int             recurs_count;
	struct recurs   recurs[RECURS_MAX];
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

struct parse_ctx *parse_ctx_init(void *_iter);
void parse_ctx_free(struct parse_ctx *ctx);

#endif /* _HAKA_LUA_PARSE_CTX_H */
