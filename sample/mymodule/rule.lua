
local mymodule = require('external/mymodule')
local ipv4 = require('protocol/ipv4')

haka.rule{
	hook = ipv4.events.receive_packet,
	eval = function ()
		mymodule.myfunc()
	end
}
