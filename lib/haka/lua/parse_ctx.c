/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>

#include <haka/config.h>
#include <haka/lua/parse_ctx.h>
#include <haka/error.h>

#ifdef HAKA_DEBUG
#define POOL_MEM_GUARD(pool) do {                                              \
	memset(&(pool)->el[(pool)->count], 0,                                  \
			((pool)->alloc - (pool)->count) * (pool)->size);       \
} while(0)
#else
#define POOL_MEM_GUARD(pool) do { } while(0)
#endif

#define POOL_INIT(elem, pool, init) do {                                       \
	(pool)->alloc = init;                                                  \
	(pool)->size = sizeof(elem);                                           \
	(pool)->el = calloc((pool)->alloc, (pool)->size);                      \
	if (!(pool)->el) {                                                     \
		error("memory error");                                         \
		return;                                                        \
	}                                                                      \
	(pool)->count = 0;                                                     \
	POOL_MEM_GUARD(pool);                                                  \
} while(0)

#define POOL_FREE(pool) do {                                                   \
	(pool)->alloc = 0;                                                     \
	free((pool)->el);                                                      \
	(pool)->count = 0;                                                     \
} while(0)

#define POOL_REALLOC(pool) do {                                                \
	if((pool)->count >= (pool)->alloc) {                                   \
		(pool)->alloc *= 2;                                            \
		(pool)->el = realloc((pool)->el, (pool)->alloc * (pool)->size);\
		if (!(pool)->el) {                                             \
			error("memory error");                                 \
			return;                                                \
		}                                                              \
		POOL_MEM_GUARD(pool);                                          \
	}                                                                      \
} while(0)

struct parse_ctx *parse_ctx_new(struct vbuffer_iterator *iter)
{
	struct parse_ctx *ctx = malloc(sizeof(struct parse_ctx));
	if (!ctx) {
		error("memory error");
		return NULL;
	}

	parse_ctx_init(ctx, iter);

	return ctx;
}

void parse_ctx_init(struct parse_ctx *ctx, struct vbuffer_iterator *iter)
{
	assert(ctx);

	ctx->run                 = true;
	ctx->current             = 1;
	ctx->next                = 1;
	ctx->lua_object          = lua_object_init;
	ctx->iter                = iter;
	ctx->compound_level      = 0;
	ctx->recurs_finish_level = 0;
	ctx->recurs_count        = 0;
	ctx->bitoffset           = 0;
	ctx->error.isset         = false;
	ctx->error.desc          = NULL;
	ctx->error.node          = 0;

	memset(ctx->recurs, 0, RECURS_MAX*sizeof(struct recurs));

	POOL_INIT(struct mark, &ctx->marks, INIT_MARKS_SIZE);
	POOL_INIT(struct catch, &ctx->catches, INIT_CATCHES_SIZE);
	POOL_INIT(struct vbuffer_iterator, &ctx->retains, INIT_RETAINS_SIZE);
}

void parse_ctx_free(struct parse_ctx *ctx)
{
	POOL_FREE(&ctx->marks);
	POOL_FREE(&ctx->catches);
	POOL_FREE(&ctx->retains);
	free(ctx->error.desc);
	free(ctx);
}

void parse_ctx_mark(struct parse_ctx *ctx, bool readonly)
{
	vbuffer_iterator_copy(ctx->iter, &ctx->retains.el[ctx->retains.count]);
	vbuffer_iterator_mark(&ctx->retains.el[ctx->retains.count], readonly);
	ctx->retains.count++;
	POOL_REALLOC(&ctx->retains);
}

void parse_ctx_unmark(struct parse_ctx *ctx)
{
	ctx->retains.count--;
	vbuffer_iterator_unmark(&ctx->retains.el[ctx->retains.count]);

#ifdef HAKA_DEBUG
	memset(&ctx->retains.el[ctx->retains.count], 0, sizeof(struct vbuffer_iterator));
#endif /* HAKA_DEBUG */
}

bool parse_ctx_get_mark(struct parse_ctx *ctx, struct vbuffer_iterator *iter)
{
	if (ctx->retains.count == 0) return false;

	vbuffer_iterator_copy(&ctx->retains.el[ctx->retains.count-1], iter);
	return true;
}

void parse_ctx_pushmark(struct parse_ctx *ctx)
{
	struct mark *mark = &ctx->marks.el[ctx->marks.count];

	vbuffer_iterator_copy(ctx->iter, &mark->iter);
	mark->bitoffset = ctx->bitoffset;
	mark->max_meter = 0;
	vbuffer_iterator_copy(ctx->iter, &mark->max_iter);
	mark->max_bitoffset = ctx->bitoffset;

	ctx->marks.count++;
	POOL_REALLOC(&ctx->marks);
}

void parse_ctx_popmark(struct parse_ctx *ctx, bool seek)
{
	ctx->marks.count--;

	if (seek) {
		struct mark *mark = &ctx->marks.el[ctx->marks.count];
		vbuffer_iterator_move(ctx->iter, &mark->max_iter);
		ctx->bitoffset = mark->max_bitoffset;
	}

#ifdef HAKA_DEBUG
	memset(&ctx->marks.el[ctx->marks.count], 0, sizeof(struct mark));
#endif /* HAKA_DEBUG */
}

void parse_ctx_seekmark(struct parse_ctx *ctx)
{
	struct mark *mark = &ctx->marks.el[ctx->marks.count-1];
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
	struct catch *catch = &ctx->catches.el[ctx->catches.count];

	/* Should have created a ctx result */
	parse_ctx_mark(ctx, false);
	parse_ctx_pushmark(ctx);

	catch->node = node;
	catch->retain_count = ctx->retains.count;
	catch->mark_count = ctx->marks.count;

	ctx->catches.count++;
	POOL_REALLOC(&ctx->catches);
}

bool parse_ctx_catch(struct parse_ctx *ctx)
{
	struct catch *catch;
	int i;

	if (ctx->catches.count == 0) {
		ctx->next = FINISH;
		return false;
	}

	catch = &ctx->catches.el[--ctx->catches.count];

	/* Unmark each retain started in failing case */
	for(i = ctx->retains.count; i > catch->retain_count; i--) {
		parse_ctx_unmark(ctx);
	}

	/* remove all marks done in failing case */
	for (i = ctx->marks.count; i > catch->mark_count; i--) {
		parse_ctx_popmark(ctx, false);
	}


	assert(!"Not Yet Implemented");
	/* remove all result ctx */
	/* for (i = ctx->result_count; i > catch->result_count; i--) {
		// TODO parse_ctx_pop();
	} */

	/* Also remove result created by Try */
	/* Should be done in Try entity but we can't */
	// parse_ctx_pop();

	parse_ctx_seekmark(ctx);
	parse_ctx_popcatch(ctx);

	ctx->next = catch->node;

	return true;
}

void parse_ctx_popcatch(struct parse_ctx *ctx)
{
	ctx->catches.count--;

	parse_ctx_popmark(ctx, false);
	parse_ctx_unmark(ctx);

#ifdef HAKA_DEBUG
	memset(&ctx->catches.el[ctx->catches.count], 0, sizeof(struct catch));
#endif /* HAKA_DEBUG */

}

void parse_ctx_error(struct parse_ctx *ctx, const char desc[])
{
	if (ctx->error.isset) {
		error("multiple parse errors raised");
		return;
	}

	ctx->error.node = ctx->current;
	ctx->error.isset = true;
	vbuffer_iterator_copy(ctx->iter, &ctx->error.iter);
	ctx->error.desc = strdup(desc);
}

bool parse_ctx_haserror(struct parse_ctx *ctx)
{
	return ctx->error.isset;
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

bool parse_ctx_get_mark_ffi(struct parse_ctx *ctx, void *_iter)
{
	return parse_ctx_get_mark(ctx, (struct vbuffer_iterator *)lua_get_swigdata(_iter));
}

struct lua_ref *parse_ctx_get_ref(void *_ctx)
{
	struct parse_ctx *ctx = (struct parse_ctx *)_ctx;
	return &ctx->lua_object.ref;
}
#endif /* HAKA_FFI */
