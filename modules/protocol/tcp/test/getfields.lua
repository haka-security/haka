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
	if proto:verify_checksum() then
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
	on = haka.dissectors.tcp.events.receive_packet,
	eval = function (pkt)
		local good, bad = checks(pkt)
		print(string.format( "----------TCP HEADER ---------"))
		print(string.format( "TCP Source Port: %d", pkt.srcport))
		print(string.format( "TCP Destination Port: %d", pkt.dstport))
		print(string.format( "TCP Sequence No: 0x%08x", pkt.seq))
		print(string.format( "TCP Ack Sequence No: 0x%08x", pkt.ack_seq))
		print(string.format( "TCP Reserved: 0x%1x", pkt.res))
		print(string.format( "TCP Header Length: %d bytes", pkt.hdr_len))
		print(string.format( "TCP Flags: 0x%02x", pkt.flags.all))
		print(string.format( "    %d... .... = FIN Flag: %s", bool(pkt.flags.fin)))
		print(string.format( "    .%d.. .... = SYN Flag: %s", bool(pkt.flags.syn)))
		print(string.format( "    ..%d. .... = RST Flag: %s", bool(pkt.flags.rst)))
		print(string.format( "    ...%d .... = PSH Flag: %s", bool(pkt.flags.psh)))
		print(string.format( "    .... %d... = ACK Flag: %s", bool(pkt.flags.ack)))
		print(string.format( "    .... .%d.. = URG Flag: %s", bool(pkt.flags.urg)))
		print(string.format( "    .... ..%d. = ECN Flag: %s", bool(pkt.flags.ecn)))
		print(string.format( "    .... ...%d = CWR Flag: %s", bool(pkt.flags.cwr)))
		print(string.format( "TCP Window Size: 0x%04x", pkt.window_size))
		print(string.format( "TCP Checksum: 0x%04x", pkt.checksum))
		print(string.format( "    %s", good))
		print(string.format( "    %s", bad))
		print(string.format( "TCP Urgent Pointer: 0x%04x", pkt.urgent_pointer))
		print()
	end
}
