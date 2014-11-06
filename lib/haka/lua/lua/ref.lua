-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <haka/config.h>

#ifdef HAKA_FFI

local ref_table, weak_ref_table, weak_id_ref_table = unpack({...})

--
-- These functions must exactly mimics luaL_ref from luajit
--

local LUA_NOREF  = -2
local LUA_REFNIL = -1

local function _ref(ref_table, obj)
	if not obj then
		return LUA_REFNIL -- Nil as an unique fixed reference
	end
	local ref = ref_table[0]
	if ref then
		-- Free element available
		-- use it
		ref_table[0] = ref_table[ref]
	else
		-- No free element
		-- create new one
		ref = #ref_table + 1
	end

	ref_table[ref] = obj
	return ref
end

local function _unref(ref_table, ref)
	ref_table[ref] = ref_table[0]
	ref_table[0] = ref
end

local function _refget(ref_table, ref)
	return ref_table[ref]
end

--
-- These functions must exactly mimics the lua_ref API from haka
--

local function ref(obj)
	return _ref(ref_table, obj)
end

local function unref(ref)
	_unref(ref_table, ref)
end

local function refget(ref)
	return _refget(ref_table, ref)
end

local function weakref(obj)
	if not obj then
		return LUA_REFNIL -- Nil as an unique fixed reference
	end
	local ref = _ref(weak_id_ref_table, true)
	weak_ref_table[ref] = obj
	return ref
end

local function unweakref(ref)
	weak_ref_table[ref] = nil
	_unref(weak_id_ref_table, ref)
end

local function weakrefget(ref)
	return weak_ref_table[ref]
end

--
-- Bind ref method to struct lua_ref
--

local ffibinding = require("ffibinding")
local ffi = require("ffi")

ffi.cdef[[
	struct lua_ref {
		void *state;
		int   ref;
		bool  weak:1;
	};
]]

local prop = {
}

local meth = {
	set = function(self, obj, weak)
		if weak then
			self.ref = weakref(obj)
			self.weak = true
		else
			self.ref = ref(obj)
			self.weak = false
		end
		self.state = haka.state
	end,
	get = function(self)
		if self.weak then
			return weakrefget(self.ref)
		else
			return refget(self.ref)
		end
	end,
	clear = function(self)
		if self.weak then
			unweakref(self.ref)
		else
			unref(self.ref)
		end
		self.ref = LUA_NOREF
		self.state = nil
	end,
	isvalid = function(self)
		return self.state ~= nil and self.ref ~= LUA_NOREF
	end
}

ffibinding.set_meta("struct lua_ref", prop, meth, {})

#endif
