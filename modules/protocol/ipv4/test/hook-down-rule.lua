
-- This test will check that ipv4-rule hook
-- is functional. See ipv4-up test too.

require("protocol/ipv4")

haka.rule {
	hook = haka.event('ipv4', 'send_packet'),
	eval = function (pkt)
		print(string.format("SRC:%s - DST:%s", pkt.src, pkt.dst))
	end
}
