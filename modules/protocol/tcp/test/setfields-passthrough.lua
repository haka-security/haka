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
require("protocol/tcp")

-- To avoid the packet to be dropped by the tcp-connection dissector,
-- disable it
haka.disable_dissector("tcp-connection")

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
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
