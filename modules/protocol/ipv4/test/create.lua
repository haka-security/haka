-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will create a new packet for scratch

local ipv4 = require("protocol/ipv4")

-- just to be safe, to avoid the test to run in an infinite loop
local counter = 10

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		if pkt.proto ~= 20 then
			if counter == 0 then
				error("loop detected")
			end
			counter = counter-1

			local npkt = haka.dissector.get('raw'):create()
			npkt = haka.dissector.get('ipv4'):create(npkt)
			npkt.version = 4
			npkt.id = 0xbeef
			npkt.flags.rb = true
			npkt.flags.df = false
			npkt.flags.mf = true
			npkt.frag_offset = 80
			npkt.ttl = 33
			npkt.proto = 20
			npkt.src = ipv4.addr(192, 168, 0, 1)
			npkt.dst = ipv4.addr("192.168.0.2")
			npkt:inject()
		end
	end
}
