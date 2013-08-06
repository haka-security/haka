-- Basic test that will output some basic information about the
-- received packets.

require("ipv4")
require("tcp")

haka.rule {
	hooks = { "tcp-connexion-new" },
	eval = function (self, pkt)
		if pkt.tcp.dstport ~= 80 then
			pkt.tcp:drop()
		end
	end
}
