-- Basic test that will output some basic information about the
-- received packets.

require("protocol/ipv4")
require("protocol/tcp")

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		pkt:compute_checksum()
	end
}
