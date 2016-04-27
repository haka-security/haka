-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will output some basic information about the
-- received packets.

local type = {
	[0] = "Echo (ping) reply",
	[8] = "Echo (ping) request",
}

local function bool(b)
	if b then return "[correct]"
	else return "[incorrect]" end
end

local icmp = require("protocol/icmp")

haka.rule {
	on = haka.dissectors.icmp.events.receive_packet,
	eval = function (pkt)
		print(string.format("Internet Control Message Protocol"))
		print(string.format("    Type: %d (%s)", pkt.type, type[pkt.type]))
		print(string.format("    Code: %d", pkt.code))
		print(string.format("    Checksum: 0x%04x %s", pkt.checksum, bool(pkt:verify_checksum())))
		print()
	end
}
