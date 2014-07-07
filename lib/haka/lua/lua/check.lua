-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local module = {}

function module.error(msg, level)
	level = level or 0
	error(msg, level+3)
end

function module.type(n, val, t, level)
	if type(val) ~= t then
		level = level or 0
		error(string.format("invalid parameter %d, expected %s", n, t), level+3)
	end
end

function module.types(n, val, types, level)
	local current_type = type(val)

	for _,t in ipairs(types) do
		if current_type == t then
			return
		end
	end

	level = level or 0
	error(string.format("invalid parameter %d, expected %s", n, table.concat(types, " or ")), level+3)
end

function module.assert(prop, message, level)
	if not prop then
		level = level or 0
		error(message, level+3)
	end
end

return module
