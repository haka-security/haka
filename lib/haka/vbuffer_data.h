/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_VBUFFER_DATA_H
#define _HAKA_VBUFFER_DATA_H

#include <haka/thread.h>

extern struct vbuffer_data_ops vbuffer_data_basic_ops;

struct vbuffer_data_basic {
	struct vbuffer_data  super;
	atomic_t             ref;
	size_t               size;
	uint8                buffer[0];
};

struct vbuffer_data_basic *vbuffer_data_basic(size_t size, bool zero);
bool                       vbuffer_data_is_basic(struct vbuffer_data *data);


struct vbuffer_data_ctl {
	struct vbuffer_data  super;
	atomic_t             ref;
};

extern struct vbuffer_data_ops vbuffer_data_ctl_select_ops;

struct vbuffer_data_ctl_select {
	struct vbuffer_data_ctl  super;
};

struct vbuffer_data_ctl_select *vbuffer_data_ctl_select();

extern struct vbuffer_data_ops vbuffer_data_ctl_push_ops;

struct vbuffer_data_ctl_push {
	struct vbuffer_data_ctl      super;
	struct vbuffer_stream       *stream;
	struct vbuffer_stream_chunk *chunk;
};

struct vbuffer_data_ctl_push   *vbuffer_data_ctl_push(struct vbuffer_stream *stream,
	struct vbuffer_stream_chunk *chunk);

extern struct vbuffer_data_ops vbuffer_data_ctl_mark_ops;

struct vbuffer_data_ctl_mark {
	struct vbuffer_data_ctl      super;
};

struct vbuffer_data_ctl_mark   *vbuffer_data_ctl_mark();

#define vbuffer_data_cast(data, type)   ((data)->ops == &type ## _ops ? (struct type*)(data) : NULL);

#endif /* _HAKA_VBUFFER_DATA_H */
