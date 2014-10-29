-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ffi = require("ffi")
local color = require("color")

local module = {}
local __path = nil

function module.load(ct)
	if ct then
		ffi.cdef(ct)
	end
	local lib = ffi.load(__path)
	__path = nil
	return lib, argv
end

function module.preload(path)
	if __path then
		error("already preloading library")
	end
	__path = path
end

function module.cdef(ct)
	return ffi.cdef(ct)
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
