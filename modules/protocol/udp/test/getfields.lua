-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will output some basic information about the
-- received packets.

require("protocol/ipv4")
local udp = require("protocol/udp")

haka.rule {
	on = haka.dissectors.udp.events.receive_packet,
	eval = function (pkt)
		print(string.format( "----------UDP HEADER ---------"))
		print(string.format( "UDP Source Port: %d", pkt.srcport))
		print(string.format( "UDP Destination Port: %d", pkt.dstport))
		print(string.format( "UDP Checksum: 0x%.04x", pkt.checksum))
		print(string.format( "UDP Length: %d", pkt.length))
	end
}
