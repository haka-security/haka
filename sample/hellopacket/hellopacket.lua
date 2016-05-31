------------------------------------
-- Loading dissectors
------------------------------------

-- Each dissector provides hooks to intercept and modify packets.
-- We need ipv4 to intercept incoming packets
-- We need tcp to intercept new connections
local ipv4 = require('protocol/ipv4')
local tcp_connection = require('protocol/tcp_connection')

-- Log info about incoming ipv4 packets
haka.rule{
	-- Rule evaluated whenever a new ipv4 packet is received
	on = haka.dissectors.ipv4.events.receive_packet,
	-- Evauation function taking ipv4 packet structure
	-- as argument
	eval = function (pkt)
		-- All fields are accessible through pkt variable.
		-- See the Haka documentation for a complete list.
		haka.log("packet from %s to %s", pkt.src, pkt.dst)
	end
}

-- Log info about connection establsihments
haka.rule{
	-- Rule evaluated at connection establishment attempt
	on = haka.dissectors.tcp_connection.events.new_connection,
	-- Rule evaluated at connection establishment attempt
	eval = function (flow, tcp)
		-- Fields from previous layer are accessible too
		haka.log("TCP connection from %s:%d to %s:%d", tcp.src,
			tcp.srcport, tcp.dst, tcp.dstport)
		-- Raise a simple alert for testing purpose
		haka.alert{
			description = "A simple alert",
			severity = "low"
		}
	end
}
