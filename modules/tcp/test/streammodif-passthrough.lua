require("ipv4")
require("tcp")

local buf = haka.stream.buffer(4)
buf[1] = 0x48
buf[2] = 0x61
buf[3] = 0x6b
buf[4] = 0x61

haka.rule {
	hooks = { "tcp-connection-up" },
	eval = function (self, pkt)
		pkt.stream:insert(buf)
	end
}

haka.rule {
	hooks = { "tcp-connection-down" },
	eval = function (self, pkt)
		pkt.stream:erase(10)
	end
}
