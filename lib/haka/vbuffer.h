/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_VBUFFER_PRIVATE_H
#define _HAKA_VBUFFER_PRIVATE_H

#include <haka/vbuffer.h>


struct vbuffer_chunk_flags {
	bool   modified:1;
	bool   writable:1;
	bool   ctl:1;
};

struct vbuffer_chunk {
	struct list2_elem           list;
	atomic_t                    ref;
	struct vbuffer_chunk_flags  flags;
	struct vbuffer_data        *data;
	vbsize                      offset;
	vbsize                      size;
};

void                  vbuffer_chunk_clear(struct vbuffer_chunk *chunk);
struct vbuffer_chunk *vbuffer_chunk_create(struct vbuffer_data *data, size_t offset, size_t length);
struct vbuffer_chunk *vbuffer_chunk_create_ctl(struct vbuffer_data *data);

#endif /* _HAKA_VBUFFER_PRIVATE_H */
