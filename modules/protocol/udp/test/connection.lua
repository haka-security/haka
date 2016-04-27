-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test to check for the udp_connection dissector

require("protocol/ipv4")
require("protocol/udp")
local udp_connection = require("protocol/udp_connection")

haka.rule{
	on = haka.dissectors.udp_connection.events.new_connection,
	eval = function (flow, pkt)
		print(string.format("New UDP connection: %s:%d -> %s:%d", flow.srcip, flow.srcport, flow.dstip, flow.dstport))
	end
}

haka.rule{
	on = haka.dissectors.udp_connection.events.receive_data,
	eval = function (flow, pkt)
		print(string.format("UDP data: %d", #pkt.payload))
	end
}

haka.rule{
	on = haka.dissectors.udp_connection.events.end_connection,
	eval = function (flow)
		print(string.format("End UDP connection: %s:%d -> %s:%d", flow.srcip, flow.srcport, flow.dstip, flow.dstport))
	end
}
