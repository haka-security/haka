
local mymodule = require('external/mymodule')
local ipv4 = require('protocol/ipv4')

haka.rule{
	on = haka.dissectors.ipv4.events.receive_packet,
	eval = function ()
		mymodule.myfunc()
	end
}
