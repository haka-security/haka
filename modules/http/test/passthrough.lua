
require("ipv4")
require("tcp")
require("http")

haka.rule {
	hooks = { "tcp-connection-new" },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = "http"
		end
	end
}

haka.rule {
	hooks = { "http-request" },
	eval = function (self, http)
		print("HTTP REQUEST")
		http.request:dump()
	end
}

haka.rule {
	hooks = { "http-response" },
	eval = function (self, http)
		print("HTTP RESPONSE")
		http.response:dump()
	end
}
