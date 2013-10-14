
-- Here we check that we still have access to IP data
-- from TCP dissected packets

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp', 'receive_packet'),
	eval = function (pkt)
		print(string.format("IP : %s:%s > %s:%s", pkt.ip.src, pkt.srcport, pkt.ip.dst, pkt.dstport))
	end
}
