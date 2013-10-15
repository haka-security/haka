
--- loading dissectors
require('protocol/ipv4')
require('protocol/tcp')
http = require('protocol/http')

--- setting the next dissector
haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		end
	end
}

