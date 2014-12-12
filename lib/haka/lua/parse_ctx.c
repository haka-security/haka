/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>

#include <haka/config.h>
#include <haka/lua/parse_ctx.h>
#include <haka/error.h>

struct parse_ctx *parse_ctx_new(struct vbuffer_iterator *iter)
{
	struct parse_ctx *ctx = malloc(sizeof(struct parse_ctx));
	if (!ctx) error("memory error");

	parse_ctx_init(ctx, iter);

	return ctx;
}

void parse_ctx_init(struct parse_ctx *ctx, struct vbuffer_iterator *iter)
{
	assert(ctx);

	ctx->run                 = true;
	ctx->node                = 1;
	ctx->lua_object          = lua_object_init;
	ctx->iter                = iter;
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

	ctx->retains = calloc(INIT_RETAINS_SIZE, sizeof(struct vbuffer_iterator));
	if (!ctx->retains) error("memory error");
	memset(ctx->retains, 0, INIT_RETAINS_SIZE*sizeof(struct vbuffer_iterator));
	ctx->retain_count = 0;

	ctx->results = calloc(INIT_RESULTS_SIZE, sizeof(struct result));
	if (!ctx->results) error("memory error");
	memset(ctx->results, 0, INIT_RESULTS_SIZE*sizeof(struct result));
	ctx->result_count = 0;
}

void parse_ctx_free(struct parse_ctx *ctx)
{
	free(ctx->marks);
	free(ctx->catches);
	free(ctx->validates);
	free(ctx->retains);
	free(ctx->results);
	free(ctx);
}

void parse_ctx_mark(struct parse_ctx *ctx, bool readonly)
{
	vbuffer_iterator_copy(ctx->iter, &ctx->retains[ctx->retain_count]);
	vbuffer_iterator_mark(&ctx->retains[ctx->retain_count], readonly);
	ctx->retain_count++;
}

void parse_ctx_unmark(struct parse_ctx *ctx)
{
	ctx->retain_count--;
	vbuffer_iterator_unmark(&ctx->retains[ctx->retain_count]);

#ifdef HAKA_DEBUG
	memset(&ctx->retains[ctx->retain_count], 0, sizeof(struct vbuffer_iterator));
#endif /* HAKA_DEBUG */
}

void parse_ctx_getmark(struct parse_ctx *ctx)
{
	/* TODO */
}

void parse_ctx_pushmark(struct parse_ctx *ctx)
{
	struct mark *mark = &ctx->marks[ctx->mark_count];

	vbuffer_iterator_copy(ctx->iter, &mark->iter);
	mark->bitoffset = ctx->bitoffset;
	mark->max_meter = 0;
	vbuffer_iterator_copy(ctx->iter, &mark->max_iter);
	mark->max_bitoffset = ctx->bitoffset;

	ctx->mark_count++;
}

void parse_ctx_popmark(struct parse_ctx *ctx, bool seek)
{
	ctx->mark_count--;

	if (seek) {
		struct mark *mark = &ctx->marks[ctx->mark_count];
		vbuffer_iterator_move(ctx->iter, &mark->max_iter);
		ctx->bitoffset = mark->max_bitoffset;
	}

#ifdef HAKA_DEBUG
	memset(&ctx->marks[ctx->mark_count], 0, sizeof(struct mark));
#endif /* HAKA_DEBUG */
}

void parse_ctx_seekmark(struct parse_ctx *ctx)
{
	struct mark *mark = &ctx->marks[ctx->mark_count-1];
	int len = ctx->iter->meter - mark->iter.meter;

	if (len >= mark->max_meter) {
		vbuffer_iterator_clear(ctx->iter);
		vbuffer_iterator_copy(&mark->max_iter, ctx->iter);
		vbuffer_iterator_register(ctx->iter);
		mark->max_meter = len;
		if (ctx->bitoffset > mark->max_bitoffset) {
			mark->max_bitoffset = ctx->bitoffset;
		}
	}

	vbuffer_iterator_move(ctx->iter, &mark->max_iter);
	ctx->bitoffset = mark->bitoffset;
}

void parse_ctx_pushcatch(struct parse_ctx *ctx, int node)
{
	struct catch *catch = &ctx->catches[ctx->catch_count];

	/* Should have created a ctx result */
	parse_ctx_mark(ctx, false);
	parse_ctx_pushmark(ctx);

	catch->node = node;
	catch->retain_count = ctx->retain_count;
	catch->mark_count = ctx->mark_count;
	catch->result_count = ctx->result_count;

	ctx->catch_count++;
}

void parse_ctx_catch(struct parse_ctx *ctx)
{
	struct catch *catch;
	int i;

	if (ctx->catch_count == 0) {
		ctx->node = FINISH;
		return;
	}

	catch = &ctx->catches[--ctx->catch_count];

	/* Unmark each retain started in failing case */
	for(i = ctx->retain_count; i > catch->retain_count; i--) {
		parse_ctx_unmark(ctx);
	}

	/* remove all marks done in failing case */
	for (i = ctx->mark_count; i > catch->mark_count; i--) {
		parse_ctx_popmark(ctx, false);
	}

	/* remove all result ctx */
	for (i = ctx->result_count; i > catch->result_count; i--) {
		// TODO parse_ctx_pop();
	}

	/* Also remove result created by Try */
	/* Should be done in Try entity but we can't */
	// TODO parse_ctx_pop();

	parse_ctx_seekmark(ctx);
	parse_ctx_popcatch(ctx);

	ctx->node = catch->node;
}

void parse_ctx_popcatch(struct parse_ctx *ctx)
{
	ctx->catch_count--;

	parse_ctx_popmark(ctx, false);
	parse_ctx_unmark(ctx);

#ifdef HAKA_DEBUG
	memset(&ctx->catches[ctx->catch_count], 0, sizeof(struct catch));
#endif /* HAKA_DEBUG */

}

#ifdef HAKA_FFI

extern void *lua_get_swigdata(void *ptr);

bool parse_ctx_new_ffi(struct ffi_object *parse_ctx, void *_iter)
{
	struct parse_ctx *ctx = parse_ctx_new((struct vbuffer_iterator *)lua_get_swigdata(_iter));
	if (!ctx) {
		return false;
	}
	else {
		parse_ctx->ref = &ctx->lua_object.ref;
		parse_ctx->ptr = ctx;
		return true;
	}
}

struct lua_ref *parse_ctx_get_ref(void *_ctx)
{
	struct parse_ctx *ctx = (struct parse_ctx *)_ctx;
	return &ctx->lua_object.ref;
}
#endif /* HAKA_FFI */
