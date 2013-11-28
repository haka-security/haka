------------------------------------
-- Use this file with hakapcap tool:
--
-- hakapcap hellopacket.pcap hellopacket.lua
--
-- (adjust paths)
------------------------------------

------------------------------------
-- Loading dissectors
------------------------------------
-- The require lines tolds Haka which dissector it has to register. Those
-- dissectors gives hook to interfere with it and some specific function
-- relative to the protocol.
require('protocol/ipv4')
require('protocol/tcp')

------------------------------------
-- This is a security rule
------------------------------------
haka.rule{
	-- The hooks tells where this rule is applied
	hooks = { 'ipv4-up' },

	-- Each rule have an eval function
	-- Eval function is always build with (self, name)
	--     First parameter must be named self
	--     Second parameter can be named whatever you want
	eval = function (self, pkt)
		-- All fields is accessible through accessors
		-- Documentation give the complet list of accessors
		haka.log("Hello", "packet from %s to %s", pkt.src, pkt.dst)
	end
}

-- Any number of rule is authorized
haka.rule{
	-- The rule is evaluated at TCP connection establishment
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		-- Fields from previous layer is accessible too
		haka.log("Hello", "TCP connection from %s:%d to %s:%d", pkt.tcp.ip.src,
			pkt.tcp.srcport, pkt.tcp.ip.dst, pkt.tcp.dstport)

		-- We can raise an alert too
		haka.alert{
			description = "A simple alert",
			severity = "low"
		}
	end
}
