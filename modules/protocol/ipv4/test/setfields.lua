-- Basic test that will output some basic information about the
-- received packets.

require("ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		pkt.version = 4
		pkt.hdr_len = 20
		pkt.len = 84
		pkt.id = 0xbeef
		pkt.flags.rb = true
		pkt.flags.df = false
		pkt.flags.mf = true
		pkt.frag_offset = 80
		pkt.ttl = 33
		pkt.proto = 17
		pkt.src = ipv4.addr(192, 168, 0, 1)
		pkt.dst = ipv4.addr("192.168.0.2")
	end
}
