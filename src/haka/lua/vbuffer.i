
%{
#include <string.h>
#include <haka/vbuffer.h>
#include <haka/error.h>

#define SUBBUFFER_TABLE "__haka_subbuffer"

/*static struct vsubbuffer *_vsubbuffer_sub(lua_State *L, struct vsubbuffer *self, int offset, int length)
{
	struct vsubbuffer *sub = malloc(sizeof(struct vsubbuffer));
	if (!sub) {
		error(L"memory error");
		return NULL;
	}

	vsubbuffer_sub(self, offset, length, sub);

	lua_getfield(L, LUA_REGISTRYINDEX, SUBBUFFER_TABLE);
	lua_pushlightuserdata(L, self);
	lua_gettable(L, -2);
	assert(!lua_isnil(L, -1));
	lua_pushlightuserdata(L, sub);
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	lua_pop(L, 2);

	return sub;
}

#define vsubbuffer_sub(self, off, len) _vsubbuffer_sub(L, self, off, len)*/

static struct vsubbuffer *_vbuffer_sub(lua_State *L, struct vbuffer *self, int offset, int length)
{
	struct vsubbuffer *sub = malloc(sizeof(struct vsubbuffer));
	if (!sub) {
		error(L"memory error");
		return NULL;
	}

	vbuffer_sub(self, offset, length, sub);

	lua_getfield(L, LUA_REGISTRYINDEX, SUBBUFFER_TABLE);
	lua_pushlightuserdata(L, sub);
	SWIG_NewPointerObj(L, self, SWIGTYPE_p_vbuffer, 0);
	lua_settable(L, -3);
	lua_pop(L, 1);

	return sub;
}

#define vbuffer_sub(self, off, len) _vbuffer_sub(L, self, off, len)
#define vbuffer_left(self, off) _vbuffer_sub(L, self, 0, off)
#define vbuffer_right(self, off) _vbuffer_sub(L, self, off, -1)

static void _vsubbuffer___gc(lua_State* L)
{
	swig_lua_userdata* usr;
	assert(lua_isuserdata(L,-1));
	usr=(swig_lua_userdata*)lua_touserdata(L,-1);
	if (usr->own) /* if must be destroyed */
	{
		lua_getfield(L, LUA_REGISTRYINDEX, SUBBUFFER_TABLE);
		lua_pushlightuserdata(L, usr->ptr);
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pop(L, 1);

		free(usr->ptr);
	}
}

#define vsubbuffer___gc(self) _vsubbuffer___gc(L)

%}

%insert("init") %{
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, SUBBUFFER_TABLE);
%}

%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%nodefaultctor;
%nodefaultdtor;

%newobject vsubbuffer::sub;

struct vsubbuffer {
	%extend {
		void __gc();
		//struct vsubbuffer *sub(int offset, int length);

		size_t __len(void *dummy)
		{
			return $self->length;
		}

		int __getitem(int index)
		{
			return vsubbuffer_getbyte($self, index-1);
		}

		void __setitem(int index, int value)
		{
			return vsubbuffer_setbyte($self, index-1, value);
		}

		int asnumber()
		{
			return vsubbuffer_asnumber($self, true);
		}

		int asnumber(const char *endian)
		{
			return vsubbuffer_asnumber($self, endian ? strcmp(endian, "big") == 0 : true);
		}

		void setnumber(int num)
		{
			return vsubbuffer_setnumber($self, true, num);
		}

		void setnumber(int num, const char *endian)
		{
			return vsubbuffer_setnumber($self, endian ? strcmp(endian, "big") == 0 : true, num);
		}

		%rename(asstring) _asstring;
		temporary_string _asstring()
		{
			char *str = malloc($self->length+1);
			if (!str) {
				error(L"memory error");
				return NULL;
			}

			vsubbuffer_asstring($self, str, $self->length);
			str[$self->length] = 0;
			return str;
		}

		%rename(setfixedstring) _setfixedstring;
		void _setfixedstring(const char *str)
		{
			vsubbuffer_setfixedstring($self, str, strlen(str));
		}

		%rename(setstring) _setstring;
		void _setstring(const char *str)
		{
			vsubbuffer_setstring($self, str, strlen(str));
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vsubbuffer);


%newobject vbuffer_iterator::sub;

struct vbuffer_iterator {
	%extend {
		~vbuffer_iterator()
		{
			free($self);
		}

		%rename(advance) _advance;
		void _advance(int size)
		{
			vbuffer_iterator_advance($self, size);
		}

		%rename(sub) _sub;
		struct vsubbuffer *_sub(int size)
		{
			struct vsubbuffer *sub = malloc(sizeof(struct vsubbuffer));
			if (!sub) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_iterator_sub($self, sub, size)) {
				free(sub);
				return NULL;
			}

			return sub;
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(vbuffer_iterator);


LUA_OBJECT(struct vbuffer);
%newobject vbuffer::sub;
%newobject vbuffer::left;
%newobject vbuffer::right;
%newobject vbuffer::iter;

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

		struct vsubbuffer *sub(int offset, int length);
		struct vsubbuffer *right(int offset);
		struct vsubbuffer *left(int offset);

		%rename(insert) _insert;
		void _insert(int offset, struct vbuffer *DISOWN_SUCCESS_ONLY)
		{
			vbuffer_insert($self, offset, DISOWN_SUCCESS_ONLY, true);
		}

		void append(struct vbuffer *DISOWN_SUCCESS_ONLY)
		{
			vbuffer_insert($self, ALL, DISOWN_SUCCESS_ONLY, true);
		}

		%rename(erase) _erase;
		void _erase(int offset, int size)
		{
			vbuffer_erase($self, offset, size);
		}

		struct vbuffer_iterator *iter()
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				error(L"memory error");
				return NULL;
			}

			if (!vbuffer_iterator($self, iter)) {
				free(iter);
				return NULL;
			}

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
