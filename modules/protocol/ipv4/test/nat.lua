
local ipv4 = require('protocol/ipv4')

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (pkt)
		if pkt.dst == ipv4.addr("10.2.253.137") then
			pkt.dst = ipv4.addr("192.168.1.137")
		end
	end
}
