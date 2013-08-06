-- Basic test that will output some basic information about the
-- received packets.

require("ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		pkt.version = 7
		pkt.len = 84
		pkt.frag_offset = 80
		pkt.ttl = 33
		pkt.proto = 17
		pkt.src = ipv4.addr(350, 350, 350, 350)
		pkt.dst = ipv4.addr("350.350.350.350")
	end
}
