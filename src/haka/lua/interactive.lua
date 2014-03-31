-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local color = require("color")

function haka.interactive_rule(self, input)
	local dump = "interactive rule\ninput = "
	local function out(...)
		dump = table.concat({dump, ...})
	end

	debug.pprint(input, "", 1, debug.hide_underscore, out)
	debug.interactive.enter(self.hook .. ">  ", self.hook .. ">> ", dump)
end
