-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will set all fields on the received packets.

local icmp = require("protocol/icmp")

haka.rule {
	on = haka.dissectors.icmp.events.receive_packet,
	eval = function (pkt)
		pkt.type = 50
		pkt.code = 12
		pkt.checksum = 1
	end
}
