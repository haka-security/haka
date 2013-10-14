
require("protocol/ipv4")

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (pkt)
		pkt.none.crash = 1
	end
}
