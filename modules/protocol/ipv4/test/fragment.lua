-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Test ipv4 reassembly

local ipv4 = require("protocol/ipv4")
local icmp = require("protocol/icmp")

haka.rule {
	on = haka.dissectors.icmp.events.receive_packet,
	eval = function (pkt)
		haka.log("received icmp packet size=%d", #pkt.payload)

		if #pkt.payload > 2000 then
			pkt.payload:pos(1476):insert(haka.vbuffer_from("HAKAHAKA"))
		end
	end
}
