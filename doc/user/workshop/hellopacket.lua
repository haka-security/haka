-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

------------------------------------
-- Loading dissectors
------------------------------------

-- Each dissector provides hooks to intercept and modify packets.
-- We need ipv4 to intercept incoming packets
-- We need tcp to intercept new connections
local ipv4 = require('protocol/ipv4')

-- Log info about incoming ipv4 packets
haka.rule{
	-- Rule evaluated whenever a new ipv4 packet is received
	hook = ipv4.events.receive_packet,
	-- Evauation function taking ipv4 packet structure
	-- as argument
	eval = function (pkt)
		-- All fields are accessible through pkt variable.
		-- See the Haka documentation for a complete list.
		haka.log("packet from %s to %s", pkt.src, pkt.dst)

		haka.alert{
			description = string.format("packet from %s to %s", pkt.src, pkt.dst),
			severity = "low"
		}
	end
}
