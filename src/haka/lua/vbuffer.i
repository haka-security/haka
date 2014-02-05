/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <string.h>
#include <haka/vbuffer.h>
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
			vbuffer_iterator_unregister($self);
			free($self);
		}

		%rename("register") _register;
		void _register()
		{
			vbuffer_iterator_register($self);
		}

		%rename(unregister) _unregister;
		void _unregister()
		{
			vbuffer_iterator_unregister($self);
		}

		%rename(advance) _advance;
		void _advance(int size)
		{
			vbuffer_iterator_advance($self, size);
		}

		%rename(clear) _clear;
		void _clear()
		{
			vbuffer_iterator_clear($self);
		}

		%rename(available) _available;
		int _available()
		{
			return vbuffer_iterator_available($self);
		}

		%rename(check_available) _check_available;
		bool _check_available(int size)
		{
			return vbuffer_iterator_check_available($self, size);
		}

		%rename(insert) _insert;
		void _insert(struct vbuffer *data)
		{
			vbuffer_iterator_insert($self, data);
		}

		%rename(erase) _erase;
		void _erase(int size)
		{
			vbuffer_iterator_erase($self, size);
		}

		%rename(replace) _replace;
		void _replace(int size, struct vbuffer *data)
		{
			vbuffer_iterator_replace($self, size, data);
		}

		%immutable;
		int offset { return $self->offset; }
		struct vbuffer *_buffer { return $self->buffer; }
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_iterator);


LUA_OBJECT(struct vbuffer);
%newobject vbuffer::iter;
%newobject vbuffer::_select;

struct vbuffer {
	%extend {
		vbuffer(size_t size)
		{
			struct vbuffer *buf = vbuffer_create_new(size);
			if (!buf) return NULL;

			vbuffer_zero(buf, true);
			return buf;
		}

		vbuffer(const char *data)
		{
			return vbuffer_create_from_string(data);
		}

		~vbuffer()
		{
			if ($self)
				vbuffer_free($self);
		}

		size_t __len(void *dummy)
		{
			return vbuffer_size($self);
		}

		int __getitem(int index)
		{
			return vbuffer_getbyte($self, index-1);
		}

		void __setitem(int index, int value)
		{
			return vbuffer_setbyte($self, index-1, value);
		}

		%rename(insert) _insert;
		void _insert(int offset, struct vbuffer *DISOWN_SUCCESS_ONLY)
		{
			if (!DISOWN_SUCCESS_ONLY) {
				error(L"invalid parameter");
				return;
			}

			vbuffer_insert($self, offset, DISOWN_SUCCESS_ONLY);
		}

		void append(struct vbuffer *DISOWN_SUCCESS_ONLY)
		{
			if (!DISOWN_SUCCESS_ONLY) {
				error(L"invalid parameter");
				return;
			}

			vbuffer_insert($self, ALL, DISOWN_SUCCESS_ONLY);
		}

		void replace(struct vbuffer *DISOWN_SUCCESS_ONLY, int off = 0, int len = -1)
		{
			if (!DISOWN_SUCCESS_ONLY) {
				error(L"invalid parameter");
				return;
			}

			vbuffer_erase($self, off, len);
			vbuffer_insert($self, off, DISOWN_SUCCESS_ONLY);
		}

		%rename(asnumber) _asnumber;
		int _asnumber(const char *endian = NULL, int off = 0, int len = -1)
		{
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off, len, &sub)) {
				return 0;
			}
			return vsubbuffer_asnumber(&sub, endian ? strcmp(endian, "big") == 0 : true);
		}

		%rename(setnumber) _setnumber;
		void _setnumber(int num, const char *endian = NULL, int off = 0, int len = -1)
		{
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off, len, &sub)) {
				return;
			}
			return vsubbuffer_setnumber(&sub, endian ? strcmp(endian, "big") == 0 : true, num);
		}

		%rename(asbits) _asbits;
		int _asbits(int off, int len, const char *endian = "big")
		{
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off / 8, (len + 7) / 8, &sub)) {
				return 0;
			}
			return vsubbuffer_asbits(&sub, off % 8, len, endian ? strcmp(endian, "big") == 0 : true);
		}

		%rename(setbits) _setbits;
		void _setbits(int num, int off, int len, const char *endian = "big")
		{
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off / 8, (len + 7) / 8, &sub)) {
				return;
			}
			return vsubbuffer_setbits(&sub, off % 8, len, endian ? strcmp(endian, "big") == 0 : true, num);
		}

		%rename(asstring) _asstring;
		temporary_string _asstring(int off = 0, int len = -1)
		{
			char *str;
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off, len, &sub)) {
				return NULL;
			}

			str = malloc(sub.length+1);
			if (!str) {
				error(L"memory error");
				return NULL;
			}

			vsubbuffer_asstring(&sub, str, sub.length);
			str[sub.length] = 0;
			return str;
		}

		%rename(setfixedstring) _setfixedstring;
		void _setfixedstring(const char *str, int off = 0, int len = -1)
		{
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off, len, &sub)) {
				return;
			}
			vsubbuffer_setfixedstring(&sub, str, strlen(str));
		}

		%rename(setstring) _setstring;
		void _setstring(const char *str, int off = 0, int len = -1)
		{
			struct vsubbuffer sub;
			if (!vbuffer_sub($self, off, len, &sub)) {
				return;
			}
			vsubbuffer_setstring(&sub, str, strlen(str));
		}

		%rename(select) _select;
		struct vbuffer *_select(int off, int len = -1, struct vbuffer **OUTPUT_DISOWN = NULL)
		{
			struct vbuffer *ret = vbuffer_select($self, off, len, OUTPUT_DISOWN);
			if (!ret) {
				return NULL;
			}
			return ret;
		}

		%rename(restore) _restore;
		void _restore(struct vbuffer *data)
		{
			vbuffer_restore($self, data);
		}

		%rename(erase) _erase;
		void _erase(int off = 0, int len = -1)
		{
			vbuffer_erase($self, off, len);
		}

		struct vbuffer_iterator *iter()
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_iterator($self, iter, 0, false, false)) {
				free(iter);
				return NULL;
			}

			//vbuffer_iterator_register(iter);
			return iter;
		}

		%immutable;
		bool modified;
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer);


LUA_OBJECT(struct vbuffer_stream);
%newobject vbuffer_stream::_pop;
%newobject vbuffer_stream::current;

struct vbuffer_stream {
	%extend {
		vbuffer_stream()
		{
		}

		~vbuffer_stream()
		{
		}

		%rename(push) _push;
		void _push(struct vbuffer *DISOWN_SUCCESS_ONLY)
		{
			vbuffer_stream_push($self, DISOWN_SUCCESS_ONLY);
		}

		%rename(pop) _pop;
		struct vbuffer *_pop()
		{
			return vbuffer_stream_pop($self);
		}

		%immutable;
		struct vbuffer *data { return $self->data; }
		struct vbuffer_iterator *current {
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}
			if (!vbuffer_stream_current($self, iter)) {
				free(iter);
				return NULL;
			}
			return iter;
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_stream);


%{
	bool vbuffer_modified_get(struct vbuffer *buf) { return vbuffer_ismodified(buf); }
%}

%luacode {
	local this = unpack({...})

	local subbuffer = class('VSubBuffer')

	function subbuffer.method:__init(buf, off, len)
		self.buf = buf
		self.off = off or 0
		self.len = len or -1
	end

	local function _sub(self, off, len)
		off = off or 0
		if self.len ~= -1 and off > self.len then
			error("invalid offset")
		end

		if len and self.len ~= -1 then
			if off + len > self.len then
				error("invalid length")
			end
		end

		return self.buf, off + self.off, len or self.len
	end

	subbuffer.method.repr = _sub

	function subbuffer.method:sub(off, len)
		local buf, off, len = _sub(self, off, len)
		return subbuffer:new(buf, off, len)
	end

	function subbuffer.method:right(off)
		local buf, off = _sub(self, off)
		return subbuffer:new(buf, off)
	end

	function subbuffer.method:left(off)
		local buf, off = _sub(self, off)
		return subbuffer:new(buf, 0, off)
	end

	function subbuffer.method:erase(off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:erase(off, len)
	end

	function subbuffer.method:insert(off, data)
		local buf, off = _sub(self, off)
		return buf:insert(off, data)
	end

	function subbuffer.method:append(data)
		local buf, off = _sub(self, off)
		return buf:append(data)
	end

	function subbuffer.method:replace(data, off, len)
		local buf, off, len = _sub(self, off, len)
		buf:erase(off, len)
		return buf:insert(off, data)
	end

	function subbuffer.method:select(off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:select(off, len)
	end

	function subbuffer.method:asnumber(endian, off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:asnumber(endian, off, len)
	end

	function subbuffer.method:setnumber(num, endian, off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:setnumber(num, endian, off, len)
	end

	function subbuffer.method:asbits(off, len, endian)
		local offB, offb = math.floor(off / 8), off % 8
		local buf, off = _sub(self, offB, math.ceil(len / 8))
		return buf:asbits(offb + off*8, len, endian)
	end

	function subbuffer.method:setbits(num, off, len, endian)
		local offB, offb = math.floor(off / 8), off % 8
		local buf, off = _sub(self, offB, math.ceil(len / 8))
		return buf:setbits(num, offb + off*8, len, endian)
	end

	function subbuffer.method:asstring(off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:asstring(off, len)
	end

	function subbuffer.method:setfixedstring(str, off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:setfixedstring(str, off, len)
	end

	function subbuffer.method:setstring(str, off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:setstring(str, off, len)
	end

	function subbuffer.method:__index(off)
		if type(off) == 'number' then
			local buf, off = _sub(self, off, 1)
			return buf[off]
		end
	end

	function subbuffer.method:__newindex(off, num)
		if type(off) == 'number' then
			local buf, off = _sub(self, off, 1)
			buf[off] = num
		end
	end

	function subbuffer:__len()
		local buf, off, len = _sub(self, 0)
		if len and len >= 0 then
			return len
		else
			return #buf - off
		end
	end

	swig.getclassmetatable('vbuffer')['.fn'].sub = function (self, off, len) return subbuffer:new(self, off, len) end
	swig.getclassmetatable('vbuffer')['.fn'].right = function (self, off) return subbuffer:new(self, off) end
	swig.getclassmetatable('vbuffer')['.fn'].left = function (self, len) return subbuffer:new(self, 0, len) end

	swig.getclassmetatable('vbuffer_iterator')['.fn'].sub = function (self, len, advance)
		advance = advance or true
		local sub = subbuffer:new(self._buffer, self.offset, len)
		if advance and len then self:advance(len) end
		return sub
	end
}
