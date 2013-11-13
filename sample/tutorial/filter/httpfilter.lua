------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
require('protocol/tcp-connection')
local http = require('protocol/http')

------------------------------------
-- Security rule
------------------------------------
haka.rule {
	hook = haka.event('tcp-connection', 'new_connection'),
	eval = function (flow, pkt)
		if pkt.dstport == 80 then
			haka.context:install_dissector(haka.dissector.get('http'):new(flow))
		else
			haka.log("Filter", "Dropping TCP connection: tcp dstpport=%d", pkt.dstport)
			-- Send a TCP RST packet to both sides: client and server
			pkt:reset()
		end
	end
}

--This rule will handle HTTP requests
haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		if string.match(request.headers['User-Agent'], 'Mozilla') then
			haka.log("Filter", "User-Agent Mozilla detected")
		else
			haka.log("Filter", "Only Mozilla User-Agent authorized")
			http:drop()
		end
	end
}
