-- Basic test that will output some basic information about the
-- received packets.

require("protocol/ipv4")
require("protocol/tcp")

haka.rule {
	hook = haka.event('tcp', 'receive_packet'),
	eval = function (pkt)
		pkt:compute_checksum()
	end
}
