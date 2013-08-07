-- Basic test that will create a new packet for scratch

local ipv4 = require("protocol/ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if pkt.proto ~= 20 then
			local npkt = haka.packet.new()
			npkt = ipv4.create(npkt)
			npkt.version = 4
			npkt.id = 0xbeef
			npkt.flags.rb = true
			npkt.flags.df = false
			npkt.flags.mf = true
			npkt.frag_offset = 80
			npkt.ttl = 33
			npkt.proto = 20
			npkt.src = ipv4.addr(192, 168, 0, 1)
			npkt.dst = ipv4.addr("192.168.0.2")
			npkt:send()
		end
	end
}
