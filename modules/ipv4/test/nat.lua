
require('ipv4')

haka2.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if pkt.dst == ipv4.addr("10.2.253.137") then
			pkt.dst = ipv4.addr("192.168.1.137")
		end
	end
}
