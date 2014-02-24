/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <string.h>
#include <haka/vbuffer.h>
#include <haka/vbuffer_stream.h>
#include <haka/error.h>

%}

%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%nodefaultctor;
%nodefaultdtor;


%newobject vbuffer_iterator::_sub;

struct vbuffer_iterator {
	%extend {
		~vbuffer_iterator()
		{
			vbuffer_iterator_clear($self);
			free($self);
		}

		void mark(bool readonly = false);
		void unmark();
		int advance(int size);

		%rename(insert) _insert;
		void _insert(struct vbuffer *data)
		{
			if (!vbuffer_iterator_isinsertable($self, data)) {
				error(L"circular buffer insertion");
				return;
			}

			vbuffer_iterator_insert($self, data);
		}

		void restore(struct vbuffer *data)
		{
			if (!vbuffer_iterator_isinsertable($self, data)) {
				error(L"circular buffer insertion");
				return;
			}

			vbuffer_restore($self, data);
		}

		int  available();

		%rename(check_available) _check_available;
		bool _check_available(int size, int *OUTPUT)
		{
			size_t available;
			bool ret = vbuffer_iterator_check_available($self, size, &available);
			*OUTPUT = available;
			return ret;
		}

		%rename(sub) _sub;
		struct vbuffer_sub *_sub(int size = ALL, bool split = false)
		{
			struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_iterator_sub($self, size, sub, split)) {
				free(sub);
				return NULL;
			}

			vbuffer_sub_register(sub);
			return sub;
		}

		%immutable;
		bool isend { return vbuffer_iterator_isend($self); }
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_iterator);


%newobject vbuffer_sub::_sub;
%newobject vbuffer_sub::pos;
%newobject vbuffer_sub::select;

struct vbuffer_sub {
	%extend {
		~vbuffer_sub()
		{
			vbuffer_sub_clear($self);
			free($self);
		}

		size_t __len(void *dummy) { return vbuffer_sub_size($self); }
		int __getitem(int index) { return vbuffer_getbyte($self, index-1); }
		void __setitem(int index, int value) { vbuffer_setbyte($self, index-1, value); }

		int  size();
		void zero() { vbuffer_zero($self); }
		void erase() { vbuffer_erase($self); }
		void replace(struct vbuffer *data)
		{
			if (!vbuffer_iterator_isinsertable(&$self->begin, data)) {
				error(L"circular buffer insertion");
				return;
			}

			vbuffer_replace($self, data);
		}
		bool isflat();

		%rename(flatten) _flatten;
		void _flatten() { vbuffer_sub_flatten($self, NULL); }

		%rename(check_size) _check_size;
		bool _check_size(int size, int *OUTPUT)
		{
			size_t available;
			bool ret = vbuffer_sub_check_size($self, size, &available);
			*OUTPUT = available;
			return ret;
		}

		struct vbuffer_iterator *select(struct vbuffer **OUTPUT)
		{
			struct vbuffer *select = malloc(sizeof(struct vbuffer));
			struct vbuffer_iterator *ref= malloc(sizeof(struct vbuffer_iterator));

			*OUTPUT = NULL;

			if (!select || !ref) {
				free(select);
				free(ref);
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_select($self, select, ref)) {
				free(select);
				free(ref);
				return NULL;
			}

			vbuffer_iterator_register(ref);
			*OUTPUT = select;
			return ref;
		}

		%rename(sub) _sub;
		struct vbuffer_sub *_sub(int offset=0, int size=ALL)
		{
			struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_sub_sub($self, offset, size, sub)) {
				free(sub);
				return NULL;
			}

			vbuffer_sub_register(sub);
			return sub;
		}

		struct vbuffer_iterator *pos(int offset = ALL)
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_sub_position($self, iter, offset)) {
				free(iter);
				return NULL;
			}

			vbuffer_iterator_register(iter);
			return iter;
		}

		int  asnumber(const char *endian = NULL) { return vbuffer_asnumber($self, endian ? strcmp(endian, "big") == 0 : true); }
		void setnumber(int value, const char *endian = NULL) { vbuffer_setnumber($self, endian ? strcmp(endian, "big") == 0 : true, value); }
		int  asbits(int offset, int bits, const char *endian = NULL) { return vbuffer_asbits($self, offset, bits, endian ? strcmp(endian, "big") == 0 : true); }
		void setbits(int offset, int bits, int value, const char *endian = NULL) { vbuffer_setbits($self, offset, bits, endian ? strcmp(endian, "big") == 0 : true, value); }

		temporary_string asstring()
		{
			const size_t len = vbuffer_sub_size($self);
			char *str = malloc(len+1);
			if (!str) {
				error(L"memory error");
				return NULL;
			}

			if (vbuffer_asstring($self, str, len+1) == (size_t)-1) {
				free(str);
				return NULL;
			}

			return str;
		}

		void setfixedstring(const char *STRING, size_t SIZE) { vbuffer_setfixedstring($self, STRING, SIZE); }
		void setstring(const char *STRING, size_t SIZE) { vbuffer_setstring($self, STRING, SIZE); }
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_sub);


LUA_OBJECT(struct vbuffer);
%newobject vbuffer::_sub;
%newobject vbuffer::pos;
%newobject vbuffer::_clone;

struct vbuffer {
	%extend {
		vbuffer(size_t size, bool zero=true)
		{
			struct vbuffer *buf = malloc(sizeof(struct vbuffer));
			if (!buf) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_create_new(buf, size, zero)) {
				free(buf);
				return NULL;
			}

			return buf;
		}

		vbuffer(const char *STRING, size_t SIZE)
		{
			struct vbuffer *buf = malloc(sizeof(struct vbuffer));
			if (!buf) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_create_from(buf, STRING, SIZE)) {
				free(buf);
				return NULL;
			}

			return buf;
		}

		~vbuffer()
		{
			if ($self) {
				vbuffer_release($self);
				free($self);
			}
		}

		size_t __len(void *dummy) { return vbuffer_size($self); }

		int __getitem(int index)
		{
			struct vbuffer_sub sub;
			vbuffer_sub_create(&sub, $self, 0, ALL);
			return vbuffer_getbyte(&sub, index-1);
		}

		void __setitem(int index, int value)
		{
			struct vbuffer_sub sub;
			vbuffer_sub_create(&sub, $self, 0, ALL);
			vbuffer_setbyte(&sub, index-1, value);
		}

		struct vbuffer_iterator *pos(int offset = ALL)
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			vbuffer_position($self, iter, offset);
			vbuffer_iterator_register(iter);
			return iter;
		}

		struct vbuffer_sub *_sub(int offset=0, int size=ALL)
		{
			struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			vbuffer_sub_create(sub, $self, offset, size);
			vbuffer_sub_register(sub);
			return sub;
		}

		%rename(append) _append;
		void _append(struct vbuffer *buffer)
		{
			if ($self == buffer) {
				error(L"circular buffer insertion");
				return;
			}

			vbuffer_append($self, buffer);
		}

		%rename(clone) _clone;
		struct vbuffer *_clone(bool copy = false)
		{
			struct vbuffer *buf = malloc(sizeof(struct vbuffer));
			if (!buf) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_clone($self, buf, copy)) {
				free(buf);
				return NULL;
			}

			return buf;
		}

		%immutable;
		bool modified { return vbuffer_ismodified($self); }
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer);


LUA_OBJECT(struct vbuffer_stream);
%newobject vbuffer_stream::_pop;
%newobject vbuffer_stream::current;

struct vbuffer_stream {
	%extend {
		~vbuffer_stream()
		{
			if ($self) {
				vbuffer_stream_clear($self);
				free($self);
			}
		}

		%rename(push) _push;
		void _push(struct vbuffer *DISOWN_SUCCESS_ONLY)
		{
			vbuffer_stream_push($self, DISOWN_SUCCESS_ONLY);
		}

		void finish();

		%rename(pop) _pop;
		struct vbuffer *_pop()
		{
			struct vbuffer *buf = malloc(sizeof(struct vbuffer));
			if (!buf) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_stream_pop($self, buf)) {
				return NULL;
			}

			return buf;
		}

		%immutable;
		struct vbuffer *data { return vbuffer_stream_data($self); }
		struct vbuffer_iterator *current {
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}
			vbuffer_stream_current($self, iter);
			return iter;
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_stream);

%luacode {
	swig.getclassmetatable('vbuffer')['.fn'].sub = swig.getclassmetatable('vbuffer')['.fn']._sub
}
