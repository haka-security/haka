-- Basic test that will output some basic information about the
-- received packets.

require("ipv4")
require("tcp")

haka2.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		pkt:computeChecksum()
	end
}
