-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Test ipv4 reassembly

local raw = require("protocol/raw")
local ipv4 = require("protocol/ipv4")
local icmp = require("protocol/icmp")

haka.rule {
	hook = icmp.events.receive_packet,
	eval = function (pkt)
		haka.log("received icmp packet data=%d", #pkt.payload)
	end
}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		haka.log("received ip packet data=%d", #pkt.payload)
	end
}
