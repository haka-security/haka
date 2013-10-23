
#ifndef _HAKA_VBUFFER_H
#define _HAKA_VBUFFER_H

#include <haka/types.h>


/*
 * Buffer
 */

struct vbuffer;

struct vbuffer_data;

struct vbuffer_data_ops {
	void   (*addref)(struct vbuffer_data *data);
	void   (*release)(struct vbuffer_data *data);
	uint8 *(*get)(struct vbuffer_data *data, bool write);
};

struct vbuffer_data {
	struct vbuffer_data_ops  *ops;
};

struct vbuffer *vbuffer_create_new(size_t size);
struct vbuffer *vbuffer_create_from(struct vbuffer_data *data, size_t length);
bool            vbuffer_recreate_from(struct vbuffer *buf, struct vbuffer_data *data, size_t length);
struct vbuffer *vbuffer_extract(struct vbuffer *buf, size_t offset, size_t length);
void            vbuffer_free(struct vbuffer *buf);
bool            vbuffer_insert(struct vbuffer *buf, size_t offset, struct vbuffer *data);
bool            vbuffer_erase(struct vbuffer *buf, size_t offset, size_t len);
bool            vbuffer_flatten(struct vbuffer *buf);
bool            vbuffer_isflat(struct vbuffer *buf);
size_t          vbuffer_size(struct vbuffer *buf);
bool            vbuffer_checksize(struct vbuffer *buf, size_t minsize);
uint8          *vbuffer_mmap(struct vbuffer *buf, void **iter, size_t *len, bool write);
struct vbuffer_data *vbuffer_mmap2(struct vbuffer *buf, void **iter, size_t *len, size_t *offset);
uint8           vbuffer_getbyte(struct vbuffer *buf, size_t offset);
void            vbuffer_setbyte(struct vbuffer *buf, size_t offset, uint8 byte);


/*
 * Iterator
 */

struct vbuffer_iterator {
	struct vbuffer  *buffer;
	size_t           offset;
};

bool            vbuffer_iterator(struct vbuffer *buf, struct vbuffer_iterator *iter);
size_t          vbuffer_iterator_read(struct vbuffer_iterator *iter, uint8 *buffer, size_t len);
bool            vbuffer_iterator_insert(struct vbuffer_iterator *iter, struct vbuffer *data);
bool            vbuffer_iterator_erase(struct vbuffer_iterator *iter, size_t len);
size_t          vbuffer_iterator_advance(struct vbuffer_iterator *iter, size_t len);


/*
 * Sub buffer
 */

struct vsubbuffer {
	struct vbuffer_iterator  position;
	size_t                   length;
};

bool            vbuffer_sub(struct vbuffer *buf, size_t offset, size_t length, struct vsubbuffer *sub);
bool            vsubbuffer_sub(struct vsubbuffer *buf, size_t offset, size_t length, struct vsubbuffer *sub);
bool            vsubbuffer_isflat(struct vsubbuffer *buf);
size_t          vsubbuffer_size(struct vsubbuffer *buf);
uint8          *vsubbuffer_mmap(struct vsubbuffer *buf, void **iter, size_t *remlen, size_t *len, bool write);
int64           vsubbuffer_asnumber(struct vsubbuffer *buf, bool bigendian);
void            vsubbuffer_setnumber(struct vsubbuffer *buf, bool bigendian, int64 num);
size_t          vsubbuffer_asstring(struct vsubbuffer *buf, char *str, size_t len);
size_t          vsubbuffer_setfixedstring(struct vsubbuffer *buf, const char *str, size_t len);
void            vsubbuffer_setstring(struct vsubbuffer *buf, const char *str, size_t len);
uint8           vsubbuffer_getbyte(struct vsubbuffer *buf, size_t offset);
void            vsubbuffer_setbyte(struct vsubbuffer *buf, size_t offset, uint8 byte);


/*
 * Buffer views
 */
/*
enum vbuffer_view_option {
	EBUFFERVIEW_BIGENDIAN,
	EBUFFERVIEW_LITTLEENDIAN,
};

enum vbuffer_view_type {
	EBUFFERVIEW_NULL        =  0,
	EBUFFERVIEW_INT8        = -1,
	EBUFFERVIEW_UINT8       =  1,
	EBUFFERVIEW_INT16       = -2,
	EBUFFERVIEW_UINT16      =  2,
	EBUFFERVIEW_INT32       = -4,
	EBUFFERVIEW_UINT32      =  4,
	EBUFFERVIEW_INT64       = -8,
	EBUFFERVIEW_UINT64      =  8,
	EBUFFERVIEW_FIXEDSTRING =  256,
	EBUFFERVIEW_STRING,
};

#define VBUFFER_VIEW_NUMBER(type) \
	struct vbuffer_view_number_##type; \
	\
	struct vbuffer_view_number_##type##_ops { \
		type     (*get)(struct vbuffer_view_number_##type *view); \
		void     (*set)(struct vbuffer_view_number_##type *view, type num); \
	}; \
	\
	struct vbuffer_view_number_##type { \
		struct vbuffer_view_##type##_ops   *ops; \
		struct vbuffer                     *buffer; \
		size_t                              offset; \
	};

VBUFFER_VIEW_NUMBER(int8);
VBUFFER_VIEW_NUMBER(uint8);
VBUFFER_VIEW_NUMBER(int16);
VBUFFER_VIEW_NUMBER(uint16);
VBUFFER_VIEW_NUMBER(int32);
VBUFFER_VIEW_NUMBER(uint32);

struct vbuffer_view_fixedstring;

struct vbuffer_view_fixedstring_ops {
	size_t   (*get)(struct vbuffer_view_fixedstring *view, char *str, size_t len);
	char    *(*get_direct)(struct vbuffer_view_fixedstring *view, size_t *len);
	size_t   (*set)(struct vbuffer_view_fixedstring *view, const char *str, size_t len);
};

struct vbuffer_view_fixedstring {
	struct vbuffer_view_fixedstring_ops   *ops;
	struct vbuffer                        *buffer;
	size_t                                 offset;
	size_t                                 length;
};

struct vbuffer_view_string;

struct vbuffer_view_string_ops {
	size_t      (*get)(struct vbuffer_view_string *view, char *str, size_t len);
	const char *(*get_direct)(struct vbuffer_view_string *view, size_t *len);
	bool        (*set)(struct vbuffer_view_string *view, const char *str, size_t len);
	bool        (*set_direct)(struct vbuffer_view_string *view, char *str, size_t len);
};

struct vbuffer_view_string {
	struct vbuffer_view_string_ops   *ops;
	struct vsubbuffer                 buffer;
	char                             *modifs;
	size_t                            modiflen;
};

struct vbuffer_view {
	enum vbuffer_view_type                 type;
	union {
		struct vbuffer_view_number_int8    int8_view;
		struct vbuffer_view_number_uint8   uint8_view;
		struct vbuffer_view_number_int16   int16_view;
		struct vbuffer_view_number_uint16  uint16_view;
		struct vbuffer_view_number_int32   int32_view;
		struct vbuffer_view_number_uint32  uint32_view;
		struct vbuffer_view_fixedstring    fixedstring_view;
		struct vbuffer_view_string         string_view;
	};
};

bool            vsubbuffer_view_create(struct vbuffer_view *view, struct vsubbuffer *buf,
		enum vbuffer_view_type type, enum vbuffer_view_option options);
*/
#endif /* _HAKA_VBUFFER_H */
