-- Basic test for the get method of dissectors
local ipv4 = require('protocol/ipv4' )
local tcp = require('protocol/tcp' )

haka.rule {
	on = haka.dissectors.tcp.events.receive_packet,
	eval = function (pkt)
		debug.pprint(pkt:get('ipv4'), { depth = 1 })
		debug.pprint(pkt:get('raw'), { depth = 1 })
	end
}
