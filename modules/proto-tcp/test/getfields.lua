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

ipv4 = require("proto-ipv4")
tcp = require("proto-tcp")

return function(pkt)

	local ip_h = ipv4(pkt)
	local tcp_h = tcp(ip_h)

	local good, bad = checks(ip_h)

	good, bad = checks(tcp_h)
	print(string.format( "----------TCP HEADER ---------"))
	print(string.format( "TCP Source Port: %d", tcp_h.srcport))
    print(string.format( "TCP Destination Port: %d", tcp_h.dstport))
    print(string.format( "TCP Sequence No: 0x%08x", tcp_h.seq))
	print(string.format( "TCP Ack Sequence No: 0x%08x", tcp_h.ack_seq))
    print(string.format( "TCP Reserved: 0x%1x", tcp_h.res))
    print(string.format( "TCP Header Length: %d bytes", tcp_h.hdr_len))
	print(string.format( "TCP Flags: 0x%02x", tcp_h.flags.all))
    print(string.format( "    %d... .... = FIN Flag: %s", bool(tcp_h.flags.fin)))
	print(string.format( "    .%d.. .... = SYN Flag: %s", bool(tcp_h.flags.syn)))
    print(string.format( "    ..%d. .... = RST Flag: %s", bool(tcp_h.flags.rst)))
    print(string.format( "    ...%d .... = PSH Flag: %s", bool(tcp_h.flags.psh)))
    print(string.format( "    .... %d... = ACK Flag: %s", bool(tcp_h.flags.ack)))
    print(string.format( "    .... .%d.. = URG Flag: %s", bool(tcp_h.flags.urg)))
    print(string.format( "    .... ..%d. = ECN Flag: %s", bool(tcp_h.flags.ecn)))
    print(string.format( "    .... ...%d = CWR Flag: %s", bool(tcp_h.flags.cwr)))
    print(string.format( "TCP Window Size: 0x%04x", tcp_h.window_size))
    print(string.format( "TCP Checksum: 0x%04x", tcp_h.checksum))
	print(string.format( "    %s", good))
	print(string.format( "    %s", bad))
    print(string.format( "TCP Urgent Pointer: 0x%04x", tcp_h.urgent_pointer))
	print()
	--tcp_h:computeChecksum()

	return packet.ACCEPT

end
