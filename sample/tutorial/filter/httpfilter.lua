--The require lines tolds Haka which dissector
--it has to register. Those dissectors gives hook
--to interfere with it.

require('protocol/ipv4')
require('protocol/tcp')
http = require('protocol/http')

--This rule will allow only TCP
--connections to port 80 (http)
haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		else
			haka.log.info("Filter", "Dropping TCP connection: tcp dstpport=%d", pkt.tcp.dstport)
			pkt:reset()
		end
	end
}

--This rule will handle HTTP communication
haka.rule {
	hooks = { 'http-request' },
	eval = function (self, http)
		if string.match(http.request.headers['User-Agent'], 'Mozilla') then
			haka.log.info("Filter", "User-Agent Mozilla detected")
		else
			haka.log.info("Filter", "Only Mozilla User-Agent authorized")
			http:drop()
		end
	end
}
