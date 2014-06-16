-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local module = {}

function module.error(msg)
	error(msg, 3)
end

function module.type(n, val, t)
	if type(val) ~= t then
		error(string.format("invalid parameter %d, expected %s", n, t), 3)
	end
end

function module.types(n, val, types)
	local current_type = type(val)

	for _,t in ipairs(types) do
		if current_type == t then
			return
		end
	end

	error(string.format("invalid parameter %d, expected %s", n, table.concat(types, " or ")), 3)
end

return module
