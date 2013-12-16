------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
local http = require('protocol/http')

------------------------------------
-- Only allow connections on port 80, close all other connections
-- Forward all accepted connections to the HTTP dissector
------------------------------------
haka.rule{
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		else
			haka.log("Filter", "Dropping TCP connection: tcp dstpport=%d",
				pkt.tcp.dstport)
			pkt:reset() -- Send a TCP RST packet to both sides: client and server
		end
	end
}

-- Only allow connections from from the 'Mozilla' user agent.
haka.rule{
	hooks = { 'http-request' },
	eval = function (self, http)
		if string.match(http.request.headers['User-Agent'], 'Mozilla') then
			haka.log("Filter", "User-Agent Mozilla detected")
		else
			haka.log("Filter", "Only Mozilla User-Agent authorized")
			http:drop()
		end
	end
}
