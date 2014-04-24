-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local color = require("color")

function haka.interactive_rule(name)
	return function (...)
		local inputs = {...}

		local dump = "interactive rule\ninputs = "
		local function out(...)
			dump = dump .. table.concat({...}, " ") .. "\n"
		end

		debug.pprint(inputs, "", 2, debug.hide_underscore, out)
		debug.interactive.enter(name .. ">  ", name .. ">> ", dump)
	end
end
