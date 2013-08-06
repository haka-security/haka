-- Basic test that will output some basic information about the
-- received packets.

require("ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		print(pcall(function () pkt.version = 4 end))
		print(pcall(function () pkt.hdr_len = 20 end))
		print(pcall(function () pkt.len = 84 end))
		print(pcall(function () pkt.id = 0xbeef end))
		print(pcall(function () pkt.flags.rb = true end))
		print(pcall(function () pkt.flags.df = false end))
		print(pcall(function () pkt.flags.mf = true end))
		print(pcall(function () pkt.frag_offset = 80 end))
		print(pcall(function () pkt.ttl = 33 end))
		print(pcall(function () pkt.proto = 17 end))
		print(pcall(function () pkt.src = ipv4.addr(192, 168, 0, 1) end))
		print(pcall(function () pkt.dst = ipv4.addr("192.168.0.2") end))
	end
}
