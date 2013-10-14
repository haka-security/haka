
-- This test will check that ipv4-rule hook
-- is functional. See ipv4-down test too.

require("protocol/ipv4")

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (pkt)
		print(string.format("SRC:%s - DST:%s", pkt.src, pkt.dst))
	end
}
