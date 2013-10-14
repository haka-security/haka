
-- This test will check that tcp-rule hook
-- is functional. See rule-up test too.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp', 'send_packet'),
	eval = function (pkt)
		print(string.format("srcport:%s - dstport:%s", pkt.srcport, pkt.dstport))
	end
}
