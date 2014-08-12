/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_VBUFFER_PRIVATE_H
#define _HAKA_VBUFFER_PRIVATE_H

#include <haka/vbuffer.h>


struct vbuffer_chunk_flags {
	bool   end:1;
	bool   eof:1;
	bool   modified:1;
	bool   writable:1;
	bool   ctl:1;
};

struct vbuffer_chunk {
	struct list2_elem           list;
	atomic_t                    ref;
	struct vbuffer_chunk_flags  flags;
	struct vbuffer_data        *data;
	vbsize_t                    offset;
	vbsize_t                    size;
};

void                  vbuffer_chunk_clear(struct vbuffer_chunk *chunk);
struct vbuffer_chunk *vbuffer_chunk_create(struct vbuffer_data *data, size_t offset, size_t length);
struct vbuffer_chunk *vbuffer_chunk_insert_ctl(struct vbuffer_chunk *ctl, struct vbuffer_data *data);
struct vbuffer_chunk *vbuffer_chunk_clone(struct vbuffer_chunk *chunk, bool copy);

struct list2         *vbuffer_chunk_list(const struct vbuffer *buf);
struct vbuffer_chunk *vbuffer_chunk_begin(const struct vbuffer *buf);
struct vbuffer_chunk *vbuffer_chunk_end(const struct vbuffer *buf);
struct vbuffer_chunk *vbuffer_chunk_next(struct vbuffer_chunk *chunk);
struct vbuffer_chunk *vbuffer_chunk_prev(struct vbuffer_chunk *chunk);
struct vbuffer_chunk *vbuffer_chunk_remove_ctl(struct vbuffer_chunk *chunk);

#define VBUFFER_FOR_EACH(buf, var) \
	for (var = vbuffer_chunk_begin(buf); var; \
		var = vbuffer_chunk_next(var))

void                  vbuffer_iterator_build(struct vbuffer_iterator *position, struct vbuffer_chunk *chunk, vbsize_t offset, vbsize_t meter);
void                  vbuffer_iterator_update(struct vbuffer_iterator *position, struct vbuffer_chunk *chunk, size_t offset);

#endif /* _HAKA_VBUFFER_PRIVATE_H */
