
-- This test will check that ipv4-rule hook
-- is functional. See ipv4-down test too.

require("protocol/ipv4")

haka.rule {
	hooks = {"ipv4-up"},
	eval = function (self,pkt)
		print(string.format("SRC:%s - DST:%s", pkt.src, pkt.dst))
	end
}
