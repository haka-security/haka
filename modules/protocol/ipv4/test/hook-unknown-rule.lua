
-- This test will check how ipv4 reacts
-- to an unkonw hook

require("protocol/ipv4")

haka.rule {
	hooks = {"ipv4-unknown"},
	eval = function (self,pkt)
		print(string.format("SRC:%s - DST:%s", pkt.src, pkt.dst))
	end
}
