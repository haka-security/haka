-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will output some basic information about the
-- received packets.

local ipv4 = require("protocol/ipv4")

haka.rule {
	on = haka.dissectors.ipv4.events.receive_packet,
	eval = function (pkt)
		print(pcall(function () pkt.version = 77 end))
		print(pcall(function () pkt.len = -84 end))
		print(pcall(function () pkt.frag_offset = 83 end))
		print(pcall(function () pkt.ttl = 333 end))
		print(pcall(function () pkt.proto = -14 end))
		print(pcall(function () pkt.src = ipv4.addr(350, 350, 350, 350) end))
		print(pcall(function () pkt.dst = ipv4.addr("350.350.350.350") end))
	end
}
