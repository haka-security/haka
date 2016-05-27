-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Test that will duplicate a tcp connection

require("protocol/raw")
local ipv4 = require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp_connection")

-- just to be safe, to avoid the test to run in an infinite loop
local counter = 10

haka.rule {
	on = haka.dissectors.tcp.events.receive_packet,
	eval = function (pkt)
		if pkt.dstport == 4444 or pkt.srcport == 4444 then
			if counter == 0 then
				error("loop detected")
			end
			counter = counter-1

			local npkt = haka.dissectors.raw.create()
			npkt = haka.dissectors.ipv4.create(npkt)
			npkt.ttl = pkt.ip.ttl
			npkt.dst = pkt.ip.dst
			npkt.src = pkt.ip.src
			npkt = haka.dissectors.tcp.create(npkt)
			npkt.window_size = pkt.window_size
			npkt.seq = pkt.seq+1000
			if pkt.ack_seq ~= 0 then
				npkt.ack_seq = pkt.ack_seq+1000
			end
			npkt.flags.all = pkt.flags.all
			npkt.dstport = pkt.dstport+10
			npkt.srcport = pkt.srcport+10

			local payload = pkt.payload:clone()
			npkt.payload:append(payload)

			npkt:inject()
		end
	end
}
