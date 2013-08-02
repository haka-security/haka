
local color = require("color")

function haka.interactive_rule(self, input)
	if not haka.__interactive then
		haka.__interactive = haka.debug.interactive()
	end

	haka.log("debug", "entering interactive rule")
	haka.debug.pprint(input, "", 1, haka.debug.hide_underscore)
	haka.__interactive:setprompt(color.green .. self.hook .. color.bold .. ">  " .. color.clear,
		color.green .. self.hook .. color.bold .. ">> " .. color.clear)
	haka.__interactive:start()
	haka.log("debug", "continue")
end
