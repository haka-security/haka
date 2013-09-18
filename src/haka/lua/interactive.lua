
local color = require("color")

function haka.interactive_rule(self, input)
	haka.log("debug", "entering interactive rule")
	haka.debug.pprint(input, "", 1, haka.debug.hide_underscore)
	haka.debug.interactive.enter(color.green .. self.hook .. color.bold .. ">  " .. color.clear,
		color.green .. self.hook .. color.bold .. ">> " .. color.clear)
	haka.log("debug", "continue")
end
