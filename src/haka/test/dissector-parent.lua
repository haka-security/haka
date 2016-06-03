-- Basic test for the get method of dissectors
local check = require('check')

local ipv4 = require('protocol/ipv4' )
local tcp = require('protocol/tcp' )

haka.rule {
	on = haka.dissectors.tcp.events.receive_packet,
	eval = function (pkt)
		check.assert(pkt:parent('ipv4') == pkt:parent())
		check.assert(pkt:parent() == pkt:parent(1))
		check.assert(pkt:parent('packet') == pkt:parent(2))

		debug.pprint(pkt:parent('ipv4'), { depth = 1 })
		debug.pprint(pkt:parent('packet'), { depth = 1 })
	end
}
