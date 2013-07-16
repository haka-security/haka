
-- This test will check that tcp-rule hook
-- is functional. See rule-up test too.

require("ipv4")
require("tcp")

haka.rule {
	hooks = {"tcp-down"},
	eval = function (self,pkt)
		print(string.format("srcport:%s - dstport:%s", pkt.srcport, pkt.dstport))
	end
}
