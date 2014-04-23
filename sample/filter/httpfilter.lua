------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
local tcp_connection = require('protocol/tcp_connection')
local http = require('protocol/http')

------------------------------------
-- Only allow connections on port 80, close all other connections
-- Forward all accepted connections to the HTTP dissector
------------------------------------
haka.rule{
	hook = tcp_connection.events.new_connection,
	eval = function (flow, pkt)
		if pkt.dstport == 80 then
			http.dissect(flow)
		else
			haka.log("Filter", "Dropping TCP connection: tcp dstpport=%d",
				pkt.dstport)
			pkt:reset() -- Send a TCP RST packet to both sides: client and server
		end
	end
}

-- Only allow connections from from the 'Mozilla' user agent.
haka.rule{
	hook = http.events.request,
	eval = function (http, request)
		if string.match(request.headers['User-Agent'], 'Mozilla') then
			haka.log("Filter", "User-Agent Mozilla detected")
		else
			haka.log("Filter", "Only Mozilla User-Agent authorized")
			http:drop()
		end
	end
}
