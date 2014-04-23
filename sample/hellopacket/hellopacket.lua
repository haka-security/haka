------------------------------------
-- This is an example lua file for the hellopacket tutorial
--
-- Use this file with hakapcap tool:
--
-- hakapcap hellopacket.pcap hellopacket.lua
--
------------------------------------

------------------------------------
-- Loading dissectors
------------------------------------
-- Each dissector provides hooks to intercept and modify packets.
-- We need ipv4 to intercept incoming packets
-- We need tcp to intercept new connectiosn
local ipv4 = require('protocol/ipv4')
require('protocol/tcp')
local tcp_connection = require('protocol/tcp_connection')

------------------------------------
-- Log all incoming packets, reporting the source and destination IP address
------------------------------------
haka.rule{
	-- Intercept all ipv4 packet before they are passed to tcp
	hook = ipv4.events.receive_packet,

	-- Function to call on all packets.
	--     self : the dissector object that handles the packet (here, ipv4 dissector)
	--     pkt : the packet that we are intercepting
	eval = function (pkt)
		-- All fields are accessible through accessors
		-- See the Haka documentation for a complete list.
		haka.log("Hello", "packet from %s to %s", pkt.src, pkt.dst)
	end
}

------------------------------------
-- Log all new connection, logging address and port of source and destination
------------------------------------
haka.rule{
	-- Intercept connection establishement, detected by the TCP dissector
	hook = tcp_connection.events.new_connection,
	eval = function (flow, tcp)
		-- Fields from previous layer are accessible too
		haka.log("Hello", "TCP connection from %s:%d to %s:%d", tcp.ip.src,
			tcp.srcport, tcp.ip.dst, tcp.dstport)

		-- Raise a simple alert for testing purpose
		haka.alert{
			description = "A simple alert",
			severity = "low"
		}
	end
}
