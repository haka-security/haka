require("ipv4")
require("tcp")

haka.rule {
	hooks = { "tcp-connection-up" },
	eval = function (self, pkt)
		haka.log.debug("filter", "received stream len=%d", pkt.stream:available())

		if pkt.stream:available() > 30 then
			pkt:reset()
		else
			local buf2 = haka.stream.buffer(4)
			buf2[1] = 0x48
			buf2[2] = 0x61
			buf2[3] = 0x6b
			buf2[4] = 0x61

			while pkt.stream:available() > 0 do
				pkt.stream:insert(buf2)
				local buf = pkt.stream:read(10)
			end
		end
	end
}
