
------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
require('protocol/tcp')
httplib = require('protocol/http')

------------------------------------
-- Setting next dissector
------------------------------------

httplib.install_tcp_rule(80)

-----------------------------------
-- Dumping request info
-----------------------------------

function dump_request(request)
	haka.log("receiving http request")
	local uri = request.uri
	haka.log("    uri: %s", uri)
	local cookies = request.headers['Cookie']
	if cookies then
		haka.log("    cookies: %s", cookies)
	end
end
