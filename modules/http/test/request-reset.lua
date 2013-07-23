require("ipv4")
require("tcp")
require("http")

haka.rule {
	hooks = { "tcp-connection-new" },
	eval = function(self, pkt)
		if pkt.tcp.dstport == 80 then
			haka.log("filter", "%s (%d) --> %s (%d)", pkt.tcp.ip.src, pkt.tcp.srcport, pkt.tcp.ip.dst, pkt.tcp.dstport)
			pkt.next_dissector = "http"
		end
	end
}

haka.rule {
	hooks = { "http-request" },
	eval = function(self, pkt)
		haka.log.debug("filter", "%s", pkt.uri)
		if #pkt.request.uri > 10 then
			pkt:reset()
		end
	end
}

