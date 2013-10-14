-- Basic test that will output some basic information about the
-- received packets.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'new_connection'),
	eval = function (flow, pkt)
		if pkt.dstport ~= 80 then
			pkt:drop()
		end
	end
}
