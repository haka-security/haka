-- Basic test that will output some basic information about the
-- received packets.

local type = {
	[0] = "Echo (ping) reply",
	[8] = "Echo (ping) request",
}

local function bool(b)
	if b then return "[correct"
	else return "[incorrect]" end
end

require("protocol/icmp")

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		print(string.format("Internet Control Message Protocol"))
		print(string.format("    Type: %d (%s)", pkt.type, type[pkt.type]))
		print(string.format("    Code: %d", pkt.code))
		print(string.format("    Checksum: 0x%04x [correct]", pkt.checksum, bool(pkt:verify_checksum())))
		print()
	end
}
