
require("protocol/ipv4")
require("protocol/tcp")

haka.rule {
	hooks = { "tcp-connection-up" },
	eval = function (self, pkt)
		haka.log.debug("filter", "received stream len=%d", pkt.stream:available())

		if pkt.stream:available() > 10 then
			pkt:reset()
		end
	end
}
