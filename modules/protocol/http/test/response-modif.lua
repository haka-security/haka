local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		print("HTTP REQUEST")
		http.request:dump()
	end
}

haka.rule {
	hook = haka.event('http', 'response'),
	eval = function (http, response)
		print("HTTP RESPONSE")
		http.response:dump()

		http.response.headers["ETag"] = nil
		http.response.headers["Haka"] = "Done"

		print("HTTP MODIFIED RESPONSE")
		http.request:dump()
	end
}
