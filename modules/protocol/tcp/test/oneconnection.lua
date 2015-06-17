-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will output some basic information about the
-- received packets.

require("protocol/ipv4")
require("protocol/tcp")
local tcp_connection = require("protocol/tcp_connection")

haka.rule{
	hook = tcp_connection.events.new_connection,
	eval = function (flow, pkt)
		haka.log("new tcp connection %s:%i -> %s:%i", pkt.ip.src, pkt.srcport,
			pkt.ip.dst, pkt.dstport)
	end
}

haka.rule{
	hook = tcp_connection.events.end_connection,
	eval = function (flow)
		haka.log("end tcp connection %s:%i -> %s:%i", flow.srcip,
			flow.srcport, flow.dstip, flow.dstport)
	end
}
