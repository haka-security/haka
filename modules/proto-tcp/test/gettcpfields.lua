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
	output:write(string.format( "----------TCP HEADER ---------\n"))
	output:write(string.format( "TCP Source Port: %d\n", tcp_h.srcport))
    output:write(string.format( "TCP Destination Port: %d\n", tcp_h.dstport))
    output:write(string.format( "TCP Sequence No: 0x%08x\n", tcp_h.seq))
	output:write(string.format( "TCP Ack Sequence No: 0x%08x\n", tcp_h.ack_seq))
    output:write(string.format( "TCP Reserved: 0x%1x\n", tcp_h.res))
    output:write(string.format( "TCP Header Length: %d bytes\n", tcp_h.hdr_len))
	output:write(string.format( "TCP Flags: 0x%02x\n", tcp_h.flags.all))
    output:write(string.format( "    %d... .... = FIN Flag: %s\n", bool(tcp_h.flags.fin)))
	output:write(string.format( "    .%d.. .... = SYN Flag: %s\n", bool(tcp_h.flags.syn)))
    output:write(string.format( "    ..%d. .... = RST Flag: %s\n", bool(tcp_h.flags.rst)))
    output:write(string.format( "    ...%d .... = PSH Flag: %s\n", bool(tcp_h.flags.psh)))
    output:write(string.format( "    .... %d... = ACK Flag: %s\n", bool(tcp_h.flags.ack)))
    output:write(string.format( "    .... .%d.. = URG Flag: %s\n", bool(tcp_h.flags.urg)))
    output:write(string.format( "    .... ..%d. = ECN Flag: %s\n", bool(tcp_h.flags.ecn)))
    output:write(string.format( "    .... ...%d = CWR Flag: %s\n", bool(tcp_h.flags.cwr)))
    output:write(string.format( "TCP Window Size: 0x%04x\n", tcp_h.window_size))
    output:write(string.format( "TCP Checksum: 0x%04x\n", tcp_h.checksum))
	output:write(string.format( "    %s\n", good))
	output:write(string.format( "    %s\n", bad))
    output:write(string.format( "TCP Urgent Pointer: 0x%04x\n", tcp_h.urgent_pointer))
	--tcp_h:computeChecksum()

	return packet.ACCEPT

end
