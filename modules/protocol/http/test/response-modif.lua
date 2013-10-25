
require("protocol/ipv4")
require("protocol/tcp")
require("protocol/http")

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

		http.response.headers["Server"] = nil
		http.response.headers["Haka"] = "Done"

		print("HTTP MODIFIED RESPONSE")
		http.response:dump()
	end
}
