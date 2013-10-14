local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		print(string.format("Ip source %s port source %s", http.connection.ipsrc, http.connection.prtsrc))
	end
}

