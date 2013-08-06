
-- This test will check that ipv4-rule hook
-- is functional. See ipv4-up test too.

require("ipv4")

haka.rule {
	hooks = {"ipv4-down"},
	eval = function (self,pkt)
		print(string.format("SRC:%s - DST:%s", pkt.src, pkt.dst))
	end
}
