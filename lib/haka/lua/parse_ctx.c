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

	ctx->retains = calloc(INIT_RETAINS_SIZE, sizeof(struct retain));
	if (!ctx->retains) error("memory error");
	memset(ctx->retains, 0, INIT_RETAINS_SIZE*sizeof(struct retain));
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
