-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will set all fields on the received packets.

local udp = require("protocol/udp")

haka.rule {
	hook = udp.events.receive_packet,
	eval = function (pkt)
		pkt.srcport = 5555
		pkt.dstport = 53
		pkt.checksum = 0xdead
	end
}
