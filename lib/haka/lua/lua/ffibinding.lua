-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local check = require("check")
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

local function destroy(cdata)
	if cdata ~= nil then
		local gc = cdata.__gc
		if gc then gc(cdata) end
	end
end

function module.create_type(arg)
	-- arg = { cdef, prop, meth, mt, destroy, ref }
	check.type(1, arg.cdef, 'string')

	local fn = {
		__ref = arg.ref,
		[".meta"] = function() return arg end
	}
	local set = {}
	arg.meth = arg.meth or {}

	ffi.cdef(string.format([[
		%s_object {
			struct lua_ref  *ref;
			 %s             *ptr;
		};
	]], arg.cdef, arg.cdef))

	local tmp = ffi.new(string.format("%s_object", arg.cdef))

	if arg.destroy then
		local __gc = arg.destroy

		object_wrapper[arg.cdef] = function (f, own)
			return function (...)
				if not f(tmp, ...) then
					return nil
				end

				local ret
				if tmp.ref:isvalid() then
					ret = tmp.ref:get()
					if own then
						ffi.gc(ret, __gc)
						tmp.ref:clear()
					end
				else
					ret = tmp.ptr
					if not own then
						tmp.ref:set(ret, false)
					else
						ffi.gc(ret, __gc)
					end
				end

				return ret
			end
		end

		arg.meth.__own = function (self)
			ffi.gc(self, __gc)
		end
	else
		object_wrapper[arg.cdef] = function (f, own)
			return function (...)
				if not f(tmp, ...) then
					return nil
				end

				local ret
				if tmp.ref:isvalid() then
					ret = tmp.ref:get()
					if own then
						tmp.ref:clear()
					end
				else
					ret = tmp.ptr
					if not own then
						tmp.ref:set(ret, false)
					end
				end

				return ret
			end
		end
	end

	if arg.prop then
		for key, value in pairs(arg.prop) do
			fn[key] = value.get
			set[key] = value.set
		end
	end

	for key, value in pairs(arg.meth) do
		fn[key] = function(self) return value end
	end

	local mt = arg.mt or {}

	mt.__index = function (self, key)
		local res = fn[key]
		if res then
			return res(self)
		else
			return nil
		end
	end

	mt.__newindex = function (self, key, value)
		local res = set[key]
		if res then
			res(self, value)
		else
			rawset(self, key, value)
		end
	end

	ffi.metatype(arg.cdef, mt)
end

function module.object_wrapper(cdef, f, own)
	return object_wrapper[cdef](f, own)
end

function module.own(cdata)
	cdata:__own()
	local ref = cdata.__ref
	assert(ref ~= nil)
	assert(ref:isvalid(), "invalid object")
	ref:clear()
end

function module.disown(cdata)
	ffi.gc(cdata, nil)
	local ref = cdata.__ref
	assert(ref ~= nil)
	assert(not ref:isvalid(), "invalid object")
	ref:set(cdata, false)
end

return module
