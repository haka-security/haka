-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will output some basic information about the
-- received packets.

local ipv4 = require("protocol/ipv4")

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		pkt.dont_reassemble = true

		pkt.version = 4
		pkt.hdr_len = 20
		pkt.len = 84
		pkt.id = 0xbeef
		pkt.flags.rb = true
		pkt.flags.df = false
		pkt.flags.mf = true
		pkt.frag_offset = 80
		pkt.ttl = 33
		pkt.proto = 17
		pkt.src = ipv4.addr(192, 168, 0, 1)
		pkt.dst = ipv4.addr("192.168.0.2")
	end
}
