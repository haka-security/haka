-- Basic test that will output some basic information about the
-- received packets.

require("ipv4")
require("tcp")

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		if pkt.dstport ~= 80 then
			pkt:drop()
		end
	end
}
