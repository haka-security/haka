/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <haka/vbuffer.h>
#include <haka/error.h>

#include "vbuffer_data.h"


void vbuffer_data_release(struct vbuffer_data *data)
{
	if (data && data->ops->release(data)) {
		data->ops->free(data);
	}
}


/*
 * Basic data
 */

#define VBUFFER_DATA_BASIC  \
	struct vbuffer_data_basic *buf = (struct vbuffer_data_basic *)_buf; \
	assert(buf->super.ops == &vbuffer_data_basic_ops)

static void vbuffer_data_basic_free(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;
	free(buf);
}

static void vbuffer_data_basic_addref(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;
	atomic_inc(&buf->ref);
}

static bool vbuffer_data_basic_release(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;
	return atomic_dec(&buf->ref) == 0;
}

static uint8 *vbuffer_data_basic_get(struct vbuffer_data *_buf, bool write)
{
	VBUFFER_DATA_BASIC;
	return buf->buffer;
}

struct vbuffer_data_ops vbuffer_data_basic_ops = {
	free:    vbuffer_data_basic_free,
	addref:  vbuffer_data_basic_addref,
	release: vbuffer_data_basic_release,
	get:     vbuffer_data_basic_get
};

struct vbuffer_data_basic *vbuffer_data_basic(size_t size, bool zero)
{
	struct vbuffer_data_basic *buf = malloc(sizeof(struct vbuffer_data_basic) + size);
	if (!buf) {
		error("memory error");
		return NULL;
	}

	buf->super.ops = &vbuffer_data_basic_ops;
	buf->size = size;
	atomic_set(&buf->ref, 0);

	if (zero) {
		memset(buf->buffer, 0, size);
	}

	return buf;
}


/*
 *  Buffer ctl data
 */

#define VBUFFER_DATA_CTL  \
	UNUSED struct vbuffer_data_ctl *buf = (struct vbuffer_data_ctl *)_buf;

static void vbuffer_data_ctl_free(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_CTL;
	free(buf);
}

static void vbuffer_data_ctl_addref(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_CTL;
	atomic_inc(&buf->ref);
}

static bool vbuffer_data_ctl_release(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_CTL;
	return atomic_dec(&buf->ref) == 0;
}

static uint8 *vbuffer_data_ctl_get(struct vbuffer_data *_buf, bool write)
{
	VBUFFER_DATA_CTL;
	return NULL;
}

static void vbuffer_data_ctl_select_free(struct vbuffer_data *_buf)
{
	struct vbuffer_data_ctl_select *select = (struct vbuffer_data_ctl_select *)_buf;
	free(select);
}

struct vbuffer_data_ops vbuffer_data_ctl_select_ops = {
	free:    vbuffer_data_ctl_select_free,
	addref:  vbuffer_data_ctl_addref,
	release: vbuffer_data_ctl_release,
	get:     vbuffer_data_ctl_get
};

struct vbuffer_data_ctl_select *vbuffer_data_ctl_select()
{
	struct vbuffer_data_ctl_select *buf = malloc(sizeof(struct vbuffer_data_ctl_select));
	if (!buf) {
		error("memory error");
		return NULL;
	}

	buf->super.super.ops = &vbuffer_data_ctl_select_ops;
	atomic_set(&buf->super.ref, 0);
	return buf;
}

struct vbuffer_data_ops vbuffer_data_ctl_push_ops = {
	free:    vbuffer_data_ctl_free,
	addref:  vbuffer_data_ctl_addref,
	release: vbuffer_data_ctl_release,
	get:     vbuffer_data_ctl_get
};

struct vbuffer_data_ctl_push *vbuffer_data_ctl_push(struct vbuffer_stream *stream,
	struct vbuffer_stream_chunk *chunk)
{
	struct vbuffer_data_ctl_push *buf = malloc(sizeof(struct vbuffer_data_ctl_push));
	if (!buf) {
		error("memory error");
		return NULL;
	}

	buf->super.super.ops = &vbuffer_data_ctl_push_ops;
	atomic_set(&buf->super.ref, 0);
	buf->stream = stream;
	buf->chunk = chunk;
	return buf;
}

struct vbuffer_data_ops vbuffer_data_ctl_mark_ops = {
	free:    vbuffer_data_ctl_free,
	addref:  vbuffer_data_ctl_addref,
	release: vbuffer_data_ctl_release,
	get:     vbuffer_data_ctl_get
};

struct vbuffer_data_ctl_mark *vbuffer_data_ctl_mark(bool readonly)
{
	struct vbuffer_data_ctl_mark *buf = malloc(sizeof(struct vbuffer_data_ctl_mark));
	if (!buf) {
		error("memory error");
		return NULL;
	}

	buf->super.super.ops = &vbuffer_data_ctl_mark_ops;
	atomic_set(&buf->super.ref, 0);
	buf->readonly = readonly;
	return buf;
}
