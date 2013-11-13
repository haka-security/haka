
local color = require("color")

function haka.interactive_rule(self, input)
	local dump = "interactive rule\ninput = "
	local function out(...)
		dump = table.concat({dump, ...})
	end

	haka.debug.pprint(input, "", 1, haka.debug.hide_underscore, out)
	haka.debug.interactive.enter(color.green .. self.hook .. color.bold .. ">  " .. color.clear,
		color.green .. self.hook .. color.bold .. ">> " .. color.clear, dump)
end
