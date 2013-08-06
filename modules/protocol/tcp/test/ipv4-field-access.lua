
-- Here we check that we still have access to IP data
-- from TCP dissected packets

require("ipv4")
require("tcp")

haka.rule {
	hooks = {"tcp-up"},
	eval = function (self,pkt)
		print(string.format("IP : %s:%s > %s:%s", pkt.ip.src, pkt.srcport, pkt.ip.dst, pkt.dstport))
	end
}
