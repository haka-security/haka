local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		print("HTTP REQUEST")
		request:dump()

		request.headers["User-Agent"] = "Haka"
		request.headers["Haka"] = "Done"

		print("HTTP MODIFIED REQUEST")
		request:dump()
	end
}

haka.rule {
	hook = haka.event('http', 'response'),
	eval = function (http, response)
		print("HTTP RESPONSE")
		response:dump()
	end
}
