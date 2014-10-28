-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ffi = require("ffi")
local color = require("color")

local module = {}


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
