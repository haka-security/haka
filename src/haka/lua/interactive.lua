-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local color = require("color")

function haka.interactive_rule(name)
	return function (...)
		local inputs = {...}

		local dump = color.green .. "interactive rule:" .. color.clear .. "\ninputs = "
		local function out(...)
			dump = dump .. table.concat({...}, " ") .. "\n"
		end

		debug.pprint(inputs, { indent = "", depth = 2, hide = debug.hide_underscore, out = out })
		debug.interactive.enter(name .. ">  ", name .. ">> ", dump)
	end
end
