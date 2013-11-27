
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

		%rename(advance) _advance;
		void _advance(int size)
		{
			vbuffer_iterator_advance($self, size);
		}

		%rename(sub) _sub;
		struct vsubbuffer *_sub(int size, bool advance = true)
		{
			struct vsubbuffer *sub = malloc(sizeof(struct vsubbuffer));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_iterator_sub($self, sub, size, advance)) {
				free(sub);
				return NULL;
			}

			vbuffer_iterator_register(&sub->position);
			return sub;
		}

		%rename(clear) _clear;
		void _clear()
		{
			vbuffer_iterator_clear($self);
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_iterator);


LUA_OBJECT(struct vbuffer);
%newobject vbuffer::iter;
%newobject vbuffer::_extract;

struct vbuffer {
	%extend {
		vbuffer(size_t size)
		{
			struct vbuffer *buf = vbuffer_create_new(size);
			if (!buf) return NULL;

			vbuffer_zero(buf, true);
			return buf;
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

			vbuffer_insert($self, offset, DISOWN_SUCCESS_ONLY, true);
		}

		void append(struct vbuffer *DISOWN_SUCCESS_ONLY, bool modify = true)
		{
			if (!DISOWN_SUCCESS_ONLY) {
				error(L"invalid parameter");
				return;
			}

			vbuffer_insert($self, ALL, DISOWN_SUCCESS_ONLY, modify);
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

		%rename(extract) _extract;
		struct vbuffer *_extract(bool modified=true, int off = 0, int len = -1)
		{
			struct vbuffer *ret = vbuffer_extract($self, off, len, modified);
			if (!ret) {
				return NULL;
			}
			return ret;
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

			if (!vbuffer_iterator($self, iter, false, false)) {
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

	function subbuffer.method:extract(modified, off, len)
		local buf, off, len = _sub(self, off, len)
		return buf:extract(modified, off, len)
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
		if len then
			return len
		else
			return #buf - off
		end
	end

	swig.getclassmetatable('vbuffer')['.fn'].sub = function (self, off, len) return subbuffer:new(self, off, len) end
	swig.getclassmetatable('vbuffer')['.fn'].right = function (self, off) return subbuffer:new(self, off) end
	swig.getclassmetatable('vbuffer')['.fn'].left = function (self, len) return subbuffer:new(self, 0, len) end
}
