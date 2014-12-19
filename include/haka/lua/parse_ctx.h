/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_PARSE_CTX_H
#define _HAKA_LUA_PARSE_CTX_H

#include <stdlib.h>
#include <string.h>

#include <haka/types.h>
#include <haka/vbuffer.h>
#include <haka/lua/object.h>

/* TODO make all these values option */
#define INIT_MARKS_SIZE     20
#define INIT_CATCHES_SIZE   20
#define INIT_VALIDATES_SIZE 20
#define INIT_RETAINS_SIZE   20
#define INIT_RESULTS_SIZE   20

#define RECURS_MAX   200
#define RECURS_NODE  0
#define RECURS_LEVEL 1

#define FINISH 0

#define POOL(elem) struct {                                                    \
	elem  *el;                                                             \
	int    count;                                                          \
	int    alloc;                                                          \
	size_t size;                                                           \
}

struct catch {
	int node;
	int retain_count;
	int mark_count;
	int result_count;
};

struct mark {
	struct vbuffer_iterator iter;
	int                     bitoffset;
	int                     max_meter;
	struct vbuffer_iterator max_iter;
	int                     max_bitoffset;
};

struct recurs {
	int node;
	int level;
};

struct parse_ctx {
	bool                          run;
	int                           node;
	int                           bitoffset;
	struct lua_object             lua_object;
	struct vbuffer_iterator      *iter;
	int                           compound_level;
	/**
	 * recurs_finish_level is the current compound level when a recursion
	 * is started. The recursion will continue when we get back to this
	 * compound level.
	 */
	int                           recurs_finish_level;
	int                           recurs_count;
	struct recurs                 recurs[RECURS_MAX];
	POOL(struct mark)             marks;
	POOL(struct catch)            catches;
	POOL(struct vbuffer_iterator) retains;

	struct {
		bool                     isset;
		struct vbuffer_iterator  iter;
		char                    *desc;
		char                    *id;
		char                    *rule;
	}                             error;
};

struct parse_ctx *parse_ctx_new(struct vbuffer_iterator *iter);
void              parse_ctx_init(struct parse_ctx *ctx, struct vbuffer_iterator *iter);
void              parse_ctx_free(struct parse_ctx *ctx);

void              parse_ctx_mark(struct parse_ctx *ctx, bool readonly);
void              parse_ctx_unmark(struct parse_ctx *ctx);
bool              parse_ctx_get_mark(struct parse_ctx *ctx, struct vbuffer_iterator *iter);
void              parse_ctx_pushmark(struct parse_ctx *ctx);
void              parse_ctx_popmark(struct parse_ctx *ctx, bool seek);
void              parse_ctx_seekmark(struct parse_ctx *ctx);
void              parse_ctx_pushcatch(struct parse_ctx *ctx, int node);
bool              parse_ctx_catch(struct parse_ctx *ctx);
void              parse_ctx_popcatch(struct parse_ctx *ctx);
void              parse_ctx_update_error(struct parse_ctx *ctx, const char id[], const char rule[]);
void              parse_ctx_error(struct parse_ctx *ctx, const char desc[]);

#ifdef HAKA_FFI
bool              parse_ctx_new_ffi(struct ffi_object *parse_ctx, void *_iter);
bool              parse_ctx_get_mark_ffi(struct parse_ctx *ctx, void *_iter);
struct lua_ref   *parse_ctx_get_ref(void *_ctx);
#endif /* HAKA_FFI */

#endif /* _HAKA_LUA_PARSE_CTX_H */
