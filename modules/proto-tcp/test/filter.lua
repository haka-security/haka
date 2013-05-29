-- Basic test that will output some basic information about the
-- received packets.

require("proto-ipv4")
require("proto-tcp")

haka2.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		if pkt.dstport ~= 80 then
			pkt:drop()
		end
	end
}
