/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <string.h>
#include <haka/vbuffer.h>
#include <haka/vbuffer_stream.h>
#include <haka/error.h>

static bool vbuffer_iterator__check_available(struct vbuffer_iterator *iter, int size, int *OUTPUT)
{
	size_t available;
	bool ret = vbuffer_iterator_check_available(iter, size, &available);
	*OUTPUT = available;
	return ret;
}

static struct vbuffer_sub *vbuffer_iterator__sub(struct vbuffer_iterator *iter, size_t size, bool split)
{
	struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
	if (!sub) {
		error(L"memory error");
		return NULL;
	}

	if (!vbuffer_iterator_sub(iter, size, sub, split)) {
		free(sub);
		return NULL;
	}

	vbuffer_sub_register(sub);
	return sub;
}

%}

%import "haka/lua/config.si"
%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%nodefaultctor;
%nodefaultdtor;

#ifdef HAKA_LUA52
/* On Lua52, it is possible to use the lua_yieldk function to implements
 * blocking operation in C.
 * It is not activated for now to avoid differences between LuaJit and
 * standard Lua.
 */
//#define USE_C_YIELD
#endif

%{

struct vbuffer_iterator_blocking {
	struct vbuffer_iterator  iterator;

#ifdef USE_C_YIELD
	struct vbuffer_iterator  begin;
	size_t                   size;
	size_t                   remsize;
	bool                     split;
#endif
};

%}

%newobject vbuffer_iterator::sub;
%newobject vbuffer_iterator::_copy;

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

		%rename(copy) _copy;
		struct vbuffer_iterator *_copy()
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			vbuffer_iterator_copy($self, iter);
			vbuffer_iterator_register(iter);
			return iter;
		}

		%rename(check_available) _check_available;
		bool _check_available(int size, int *OUTPUT);

		struct vbuffer_sub *sub(int size, bool split = false) { return vbuffer_iterator__sub($self, size, split); }

		struct vbuffer_sub *sub(const char *mode, bool split = false)
		{
			if (!mode) {
				error(L"missing mode parameter");
				return NULL;
			}

			if (strcmp(mode, "available") == 0 ||
			    strcmp(mode, "all") == 0) {
				return vbuffer_iterator__sub($self, ALL, split);
			}
			else {
				error(L"unknown sub buffer mode: %s", mode);
				return NULL;
			}
		}

		%immutable;
		bool isend { return vbuffer_iterator_isend($self); }
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_iterator);


%{

#ifdef USE_C_YIELD

static int _advance_blocking_until(lua_State *L, struct vbuffer_iterator_blocking *self,
		struct vbuffer_iterator *update_iter, lua_CFunction cont)
{
	size_t adv;
	struct vbuffer_iterator iter;

	if (update_iter) {
		vbuffer_iterator_clear(&self->iterator);
		vbuffer_iterator_copy(update_iter, &self->iterator);
		vbuffer_iterator_register(&self->iterator);
	}

	vbuffer_iterator_copy(&self->iterator, &iter);

	adv = vbuffer_iterator_advance(&self->iterator, self->remsize);
	if (adv > 0 && !vbuffer_iterator_isvalid(&self->begin)) {
		vbuffer_iterator_copy(&iter, &self->begin);
		vbuffer_iterator_register(&self->begin);
	}

	self->remsize -= adv;

	if (self->remsize > 0 && !vbuffer_iterator_isend(&self->iterator)) {
		lua_pop(L, 1);
		return lua_yieldk(L, 0, 0, cont);
	}

	return 0;
}

static int _advance_blocking(lua_State *L, struct vbuffer_iterator_blocking *self,
		struct vbuffer_iterator *update_iter, lua_CFunction cont)
{
	size_t adv;
	struct vbuffer_iterator iter;

	if (update_iter) {
		vbuffer_iterator_clear(&self->iterator);
		vbuffer_iterator_copy(update_iter, &self->iterator);
		vbuffer_iterator_register(&self->iterator);
	}

	vbuffer_iterator_copy(&self->iterator, &iter);

	adv = vbuffer_iterator_advance(&self->iterator, self->remsize);
	if (adv > 0 && !vbuffer_iterator_isvalid(&self->begin)) {
		vbuffer_iterator_copy(&iter, &self->begin);
		vbuffer_iterator_register(&self->begin);
	}

	if (adv == 0 && !vbuffer_iterator_isend(&self->iterator)) {
		lua_pop(L, 1);
		return lua_yieldk(L, 0, 0, cont);
	}

	self->remsize -= adv;
	return 0;
}

static int _wrap_vbuffer_iterator_blocking__advance_blocking(lua_State* L);
static int _wrap_vbuffer_iterator_blocking__sub_blocking(lua_State* L);
static int _wrap_vbuffer_iterator_blocking__sub_available_blocking(lua_State* L);

#endif

%}

%newobject vbuffer_iterator_blocking::sub;
%newobject vbuffer_iterator_blocking::_sub_blocking;
%newobject vbuffer_iterator_blocking::_sub_available_blocking;

struct vbuffer_iterator_blocking {
	%extend {
		vbuffer_iterator_blocking(struct vbuffer_iterator *src)
		{
			struct vbuffer_iterator_blocking *iter = malloc(sizeof(struct vbuffer_iterator_blocking));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

#ifdef USE_C_YIELD
			iter->begin = vbuffer_iterator_init;
#endif
			vbuffer_iterator_copy(src, &iter->iterator);
			vbuffer_iterator_register(&iter->iterator);
			return iter;
		}

		~vbuffer_iterator_blocking()
		{
#ifdef USE_C_YIELD
			vbuffer_iterator_clear(&$self->begin);
#endif
			vbuffer_iterator_clear(&$self->iterator);
			free($self);
		}

		void mark(bool readonly = false) { vbuffer_iterator_mark(&$self->iterator, readonly); }
		void unmark() { vbuffer_iterator_unmark(&$self->iterator); }
		void insert(struct vbuffer *data) { vbuffer_iterator_insert(&$self->iterator, data); }
		int  available() { return vbuffer_iterator_available(&$self->iterator); }
		bool check_available(int size, int *OUTPUT) { vbuffer_iterator__check_available(&$self->iterator, size, OUTPUT); }

#ifdef USE_C_YIELD
		int  _advance_blocking(lua_State *L, struct vbuffer_iterator *update_iter, bool *YIELD)
		{
			const int ret = _advance_blocking_until(L, $self, update_iter, _wrap_vbuffer_iterator_blocking__advance_blocking);
			if (YIELD) *YIELD = (ret == -1);

			vbuffer_iterator_clear(&$self->begin);
			return $self->size - $self->remsize;
		}

		struct vbuffer_sub *_sub_blocking(lua_State *L, struct vbuffer_iterator *update_iter, bool *YIELD)
		{
			const int ret = _advance_blocking_until(L, $self, update_iter, _wrap_vbuffer_iterator_blocking__sub_blocking);
			if (YIELD) *YIELD = (ret == -1);

			if (ret >= 0 && self->remsize < self->size) {
				struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
				if (!sub) {
					error(L"memory error");
					vbuffer_iterator_clear(&$self->begin);
					return NULL;
				}

				if (self->split) {
					vbuffer_iterator_split(&self->iterator);
				}

				if (!vbuffer_sub_create_between_position(sub, &self->begin, &self->iterator)) {
					free(sub);
					vbuffer_iterator_clear(&$self->begin);
					return NULL;
				}

				vbuffer_sub_register(sub);
				vbuffer_iterator_clear(&$self->begin);
				return sub;
			}

			return NULL;
		}

		struct vbuffer_sub *_sub_available_blocking(lua_State *L, struct vbuffer_iterator *update_iter, bool *YIELD)
		{
			const int ret = _advance_blocking(L, $self, update_iter, _wrap_vbuffer_iterator_blocking__sub_available_blocking);
			if (YIELD) *YIELD = (ret == -1);

			if (ret >= 0 && self->remsize < self->size) {
				struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
				if (!sub) {
					error(L"memory error");
					vbuffer_iterator_clear(&$self->begin);
					return NULL;
				}

				if (self->split) {
					vbuffer_iterator_split(&self->iterator);
				}

				if (!vbuffer_sub_create_between_position(sub, &self->begin, &self->iterator)) {
					free(sub);
					vbuffer_iterator_clear(&$self->begin);
					return NULL;
				}

				vbuffer_sub_register(sub);
				vbuffer_iterator_clear(&$self->begin);
				return sub;
			}

			return NULL;
		}

		int  advance(lua_State *L, int size, bool *YIELD)
		{
			$self->size = size;
			$self->remsize = size;
			return vbuffer_iterator_blocking__advance_blocking($self, L, NULL, YIELD);
		}

		struct vbuffer_sub *sub(lua_State *L, int size, bool split = false, bool *YIELD)
		{
			$self->size = size;
			$self->remsize = size;
			$self->split = split;
			return vbuffer_iterator_blocking__sub_blocking($self, L, NULL, YIELD);
		}

		struct vbuffer_sub *sub(lua_State *L, const char *mode, bool split = false, bool *YIELD)
		{
			if (!mode) {
				error(L"missing mode parameter");
				return NULL;
			}

			if (strcmp(mode, "all") == 0) {
				$self->size = ALL;
				$self->remsize = ALL;
				$self->split = split;
				return vbuffer_iterator_blocking__sub_blocking($self, L, NULL, YIELD);
			}
			else if (strcmp(mode, "available") == 0) {
				$self->size = ALL;
				$self->remsize = ALL;
				$self->split = split;
				return vbuffer_iterator_blocking__sub_available_blocking($self, L, NULL, YIELD);
			}
			else {
				error(L"unknown sub buffer mode: %s", mode);
				return NULL;
			}
		}

#else
		void _update_iter(struct vbuffer_iterator *update_iter)
		{
			vbuffer_iterator_clear(&self->iterator);
			vbuffer_iterator_copy(update_iter, &self->iterator);
			vbuffer_iterator_register(&self->iterator);
		}

		%immutable;
		struct vbuffer_iterator *_iter { return &$self->iterator; }
#endif

		%immutable;
		bool isend { return vbuffer_iterator_isend(&$self->iterator); }
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_iterator_blocking);


%newobject vbuffer_sub::sub;
%newobject vbuffer_sub::pos;
%newobject vbuffer_sub::select;

struct vbuffer_sub {
	%extend {
		vbuffer_sub(struct vbuffer_iterator *begin, struct vbuffer_iterator *end)
		{
			struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_sub_create_between_position(sub, begin, end)) {
				free(sub);
				return NULL;
			}

			vbuffer_sub_register(sub);
			return sub;
		}

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

		struct vbuffer_sub *sub(int offset, int size=-1)
		{
			struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_sub_sub($self, offset, size == -1 ? ALL : size, sub)) {
				free(sub);
				return NULL;
			}

			vbuffer_sub_register(sub);
			return sub;
		}

		struct vbuffer_sub *sub(int offset, const char *mode)
		{
			if (!mode) {
				error(L"missing mode parameter");
				return NULL;
			}

			if (strcmp(mode, "all") == 0) {
				return vbuffer_sub_sub__SWIG_0($self, offset, -1);
			}
			else {
				error(L"unknown sub buffer mode: %s", mode);
				return NULL;
			}
		}

		struct vbuffer_iterator *pos(int offset)
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_sub_position($self, iter, offset == -1 ? ALL : offset)) {
				free(iter);
				return NULL;
			}

			vbuffer_iterator_register(iter);
			return iter;
		}

		struct vbuffer_iterator *pos(const char *pos)
		{
			if (!pos) {
				error(L"missing pos parameter");
				return NULL;
			}

			if (strcmp(pos, "begin") == 0) return vbuffer_sub_pos__SWIG_0($self, 0);
			else if (strcmp(pos, "end") == 0) return vbuffer_sub_pos__SWIG_0($self, -1);
			else {
				error(L"unknown buffer position: %s", pos);
				return NULL;
			}
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
%newobject vbuffer::sub;
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

		struct vbuffer_iterator *pos(int offset)
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			vbuffer_position($self, iter, offset == -1 ? ALL : offset);
			vbuffer_iterator_register(iter);
			return iter;
		}

		struct vbuffer_iterator *pos(const char *pos)
		{
			if (!pos) {
				error(L"missing pos parameter");
				return NULL;
			}

			if (strcmp(pos, "begin") == 0) return vbuffer_pos__SWIG_0($self, 0);
			else if (strcmp(pos, "end") == 0) return vbuffer_pos__SWIG_0($self, -1);
			else {
				error(L"unknown buffer position: %s", pos);
				return NULL;
			}
		}

		struct vbuffer_sub *sub(int offset, int size=-1)
		{
			struct vbuffer_sub *sub = malloc(sizeof(struct vbuffer_sub));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			vbuffer_sub_create(sub, $self, offset, size == -1 ? ALL : size);
			vbuffer_sub_register(sub);
			return sub;
		}

		struct vbuffer_sub *sub(int offset, const char *mode)
		{
			if (!mode) {
				error(L"missing mode parameter");
				return NULL;
			}

			if (strcmp(mode, "all") == 0) {
				return vbuffer_sub__SWIG_0($self, offset, -1);
			}
			else {
				error(L"unknown sub buffer mode: %s", mode);
				return NULL;
			}
		}

		struct vbuffer_sub *sub()
		{
			return vbuffer_sub__SWIG_0($self, 0, -1);
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
		vbuffer_stream()
		{
			struct vbuffer_stream *stream = malloc(sizeof(struct vbuffer_stream));
			if (!stream) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_stream_init(stream)) {
				free(stream);
				return NULL;
			}

			return stream;
		}

		~vbuffer_stream()
		{
			vbuffer_stream_clear($self);
			free($self);
		}

		%rename(push) _push;
		void _push(struct vbuffer *data)
		{
			vbuffer_stream_push($self, data);
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
			vbuffer_iterator_register(iter);
			return iter;
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_stream);

#ifndef USE_C_YIELD
%luacode {
	swig.getclassmetatable('vbuffer_iterator_blocking')['.fn'].advance = function (self, size)
		local remsize = size
		while true do
			local adv = self._iter:advance(remsize)
			remsize = remsize-adv
			if remsize == 0 or self.isend then break end

			local iter = coroutine.yield()
			self:_update_iter(iter)
		end

		return size-remsize
	end

	swig.getclassmetatable('vbuffer_iterator_blocking')['.fn'].sub = function (self, size_or_mode, split)
		local size
		local available = false

		if size_or_mode == 'available' then
			available = true
		elseif size_or_mode == 'all' then
		else
			size = tonumber(size_or_mode)
		end

		if size == 0 then
			return self._iter:sub(0)
		end

		local remsize = size or -1
		local begin
		local iter = self._iter:copy()

		while true do
			local adv = self._iter:advance(remsize or -1)
			if not begin and adv > 0 then begin = iter end

			if remsize then remsize = remsize-adv end
			if self.isend then break end

			if available then
				if adv > 0 then break end
			else
				if remsize == 0 then break end
			end

			iter = coroutine.yield()
			self:_update_iter(iter)
		end

		if begin then
			return haka.vbuffer_sub(begin, self._iter)
		else
			return nil
		end
	end
}
#endif

%luacode {
	haka.vbuffer_stream_comanager = class('VbufferStreamCoManager')

	function haka.vbuffer_stream_comanager.method:__init(stream)
		self._co = {}
		self._stream = stream
	end

	local function wrapper(manager, f)
		return function (iter)
			local blocking_iter = haka.vbuffer_iterator_blocking(iter)
			local ret, msg = xpcall(function () f(blocking_iter) end, debug.format_error)
			if not ret then
				manager._error = msg
			end
		end
	end

	function haka.vbuffer_stream_comanager.method:start(f)
		self._co[f] = coroutine.create(wrapper(self, f))
	end

	local function process_one(self, f, co)
		coroutine.resume(co, self._stream.current)
		if self._error then
			error(self._error)
		end

		if coroutine.status(co) == "dead" then
			self._co[f] = false
		end
	end

	function haka.vbuffer_stream_comanager.method:process(f)
		if self._co[f] == nil then
			self:start(f)
		end

		if self._co[f] then
			return process_one(self, f, self._co[f])
		end
	end

	function haka.vbuffer_stream_comanager.method:process_all()
		for f,co in pairs(self._co) do
			process_one(self, f, co)
		end
	end
}
