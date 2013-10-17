-- Basic test that will output some basic information about the
-- received packets.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule{
	hook = haka.event('tcp-connection', 'new_connection'),
	eval = function (flow, pkt)
		haka.log("test", "new tcp connection %s:%i -> %s:%i", pkt.ip.src, pkt.srcport,
			pkt.ip.dst, pkt.dstport)
	end
}

haka.rule{
	hook = haka.event('tcp-connection', 'end_connection'),
	eval = function (flow)
		haka.log("test", "end tcp connection %s:%i -> %s:%i", flow.connection.srcip,
			flow.connection.srcport, flow.connection.dstip, flow.connection.dstport)
	end
}
