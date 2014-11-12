-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ffi = require("ffi")
local color = require("color")

local module = {}
local __path = nil

ffi.cdef[[
	const char *clear_error();
	struct lua_ref *lua_object_get_ref(void *_obj);
]]

function module.load(ct)
	if ct then
		ffi.cdef(ct)
	end

	local lib
	if __path then
		lib = ffi.load(__path)
		__path = nil
	else
		lib = ffi.C
	end
	return lib
end

function module.preload(path)
	if __path then
		error("already preloading library")
	end
	__path = path
end

function module.handle_error(fn)
	if type(fn) ~= 'cdata' then
		error("cannot handle error on not ffi function")
	end

	return function(...)
		local ret = fn(...)
		local error_str = ffi.C.clear_error()
		if error_str ~= nil then
			error(ffi.string(error_str))
		end
		return ret
	end
end

local object_wrapper = {}

function module.create_type(cdef, prop, meth, mt, destroy)
	local fn = {
		__ref = ffi.C.lua_object_get_ref,
		__gc = function () return destroy end,
		[".meta"] = function() return { prop = prop, meth = meth, mt = mt } end
	}
	local set = {}

	for key, value in pairs(prop) do
		fn[key] = value.get
		set[key] = value.set
	end

	for key, value in pairs(meth) do
		fn[key] = function(self) return value end
	end

	mt.__index = function (table, key)
		local res = fn[key]
		if res then
			return res(table)
		else
			return nil
		end
	end

	mt.__newindex = function (table, key, value)
		local res = set[key]
		if res then
			res(table, value)
		else
			rawset(table, key, value)
		end
	end

	local ctype = ffi.metatype(cdef, mt)

	ffi.cdef(string.format([[
		%s_object {
			struct lua_ref  *ref;
			 %s             *ptr;
		};
	]], cdef, cdef))

	local tmp = ffi.new(string.format("%s_object", cdef))

	object_wrapper[cdef] = function (f)
		return function (...)
			f(tmp, ...)

			if tmp.ref == nil then return nil end

			if tmp.ref:isvalid() then
				return tmp.ref:get()
			else
				local ret = tmp.ptr
				haka.C.register_object(tmp.ref, ret)
				return ret
			end
		end
	end
end

function module.object_wrapper(cdef)
	return object_wrapper[cdef]
end

local function destroy(cdata)
	local gc = cdata.__gc
	if gc then gc(cdata) end
end

function module.own(cdata)
	ffi.gc(cdata, destroy)
	local ref = cdata.__ref
	assert(ref:isvalid(), "invalid object")
	if not cdata.__ref.weak then
		cdata.__ref:clear()
		cdata.__ref:set(cdata, true)
	end
end

function module.disown(cdata)
	ffi.gc(cdata, nil)
	local ref = cdata.__ref
	assert(ref:isvalid(), "invalid object")
	if cdata.__ref.weak then
		cdata.__ref:clear()
		cdata.__ref:set(cdata, false)
	end
end

return module
