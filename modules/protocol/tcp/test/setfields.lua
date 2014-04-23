-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will output some basic information about the
-- received packets.

function bool(val)
	if(val) then
		return 1, "Set"
	end
	return 0, "Not set"
end

function checks(proto)
	if proto:verifyChecksum() then
		good = "[Good: True]"
		bad = "[Bad: False]"
	else
		good = "[Good: False]"
		bad = "[Bad: True]"
	end
	return good, bad
end

require("protocol/ipv4")
local tcp = require("protocol/tcp")

haka.rule {
	hook = tcp.events.receive_packet,
	eval = function (pkt)
		pkt.srcport = 3
		pkt.dstport = 65535
		pkt.seq = 123456
		pkt.ack_seq = 654321
		pkt.res = 0
		pkt.hdr_len = 20
		pkt.flags.fin = true
		pkt.flags.syn = true
		pkt.flags.rst = true
		pkt.flags.psh = true
		pkt.flags.ack = true
		pkt.flags.urg = true
		pkt.flags.ecn = true
		pkt.flags.cwr = true
		pkt.window_size = 32
		pkt.urgent_pointer = 15
	end
}
