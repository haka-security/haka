-- Basic test that will output some basic information about the
-- received packets.

ipv4 = require("proto-ipv4")

return function(pkt)
	ip = ipv4(pkt)
	ip.version = 4
	ip.hdr_len = 20
	ip.len = 84
	ip.id = 0xbeef
	ip.flags.rb = true
	ip.flags.df = false
	ip.flags.mf = true
	ip.frag_offset = 80
	ip.ttl = 33
	ip.proto = 17
	ip.src = ipv4.addr(192, 168, 0, 1)
	ip.dst = ipv4.addr("192.168.0.2")
	ip:forge()

	return packet.ACCEPT
end

