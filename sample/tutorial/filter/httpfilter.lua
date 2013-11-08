------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
local http = require('protocol/http')

------------------------------------
-- Security rule
------------------------------------
haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		else
			haka.log("Filter", "Dropping TCP connection: tcp dstpport=%d", pkt.tcp.dstport)
			-- Send a TCP RST packet to both sides: client and server
			pkt:reset()
		end
	end
}

--This rule will handle HTTP requests
haka.rule {
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
