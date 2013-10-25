-- Basic test that will set all fields on the received packets.

require("protocol/icmp")

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		pkt.type = 50
		pkt.code = 12
		pkt.checksum = 1
	end
}
