require("protocol/ipv4")
require("protocol/tcp")
local http = require("protocol/http")

http.install_tcp_rule(80)


haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		local uri = request.uri
		print('--> splitting uri')
		splitted = request:split_uri()
		print(splitted)
	end
}

haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		local uri = request.uri
		print('--> splitting uri')
		local splitted2 = request:split_uri()
		print(splitted2)
		--Check that tables are the same
		--Same in the way that this is the same memory, not only the same content 
		if splitted == splitted2
		then
			print("Table are the same")
		else
			print("Table are different")
		end
	end
}

