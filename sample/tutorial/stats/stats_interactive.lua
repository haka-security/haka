
require('stats')
local color = require('color')

haka.on_exit(function()
	haka.debug.interactive.enter(color.green .. color.bold .. ">  " .. color.clear, color.green .. color.bold .. ">> " .. color.clear, "entering interactive mode for playing statistics\n")
end)

