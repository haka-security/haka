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
		print(pcall(function () pkt.srcport = 3 end))
		print(pcall(function () pkt.dstport = 65535 end))
		print(pcall(function () pkt.seq = 123456 end))
		print(pcall(function () pkt.ack_seq = 654321 end))
		print(pcall(function () pkt.res = 0 end))
		print(pcall(function () pkt.hdr_len = 20 end))
		print(pcall(function () pkt.flags.fin = true end))
		print(pcall(function () pkt.flags.syn = true end))
		print(pcall(function () pkt.flags.rst = true end))
		print(pcall(function () pkt.flags.psh = true end))
		print(pcall(function () pkt.flags.ack = true end))
		print(pcall(function () pkt.flags.urg = true end))
		print(pcall(function () pkt.flags.ecn = true end))
		print(pcall(function () pkt.flags.cwr = true end))
		print(pcall(function () pkt.window_size = 32 end))
		print(pcall(function () pkt.urgent_pointer = 15 end))
	end
}
