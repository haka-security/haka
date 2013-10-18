
------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
require('protocol/tcp')
http = require('protocol/http')

------------------------------------
-- Setting next dissector
------------------------------------

haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		end
	end
}

-----------------------------------
-- Dumping request info
-----------------------------------

function dump_request(http)
	haka.log("sqli", "receiving http request")
	local uri = http.request.uri
	haka.log("sqli", "    uri: %s", uri)
	local cookies = http.request.headers['Cookie']
	if cookies then
		haka.log("sqli", "    cookies: %s", cookies)
	end
end


