-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ffi = require("ffi")
local color = require("color")

local module = {}
local __path = nil

ffi.cdef[[
	const char *clear_error();
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

function module.set_meta(cdef, prop, meth, mt)
	local fn = { }
	fn[".meta"] = function() return { prop = prop, meth = meth, mt = mt } end

	for key, value in pairs(prop) do
		fn[key] = value
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

	local ctype = ffi.metatype(cdef, mt)
end

return module
