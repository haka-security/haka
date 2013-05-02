-- Basic test that will output some basic information about the
-- received packets.

function bool(val)
	if val then
		return 1, "Set"
	else
		return 0, "Not set"
	end
end

function protocol(val)
	if val == 1 then
		return "ICMP"
	end
	if val == 6 then
		return "TCP"
	end
	if val == 17 then
		return "UDP"
	else
		return ""
	end
end

function checks(ip)
	if ip:verifyChecksum() then
		good = "[Good: True]"
		bad = "[Bad: False]"
		msg = "[correct]"
	else
		good = "[Good: False]"
		bad = "[Bad: True]"
		msg = "[incorrect]"
	end
	return good, bad, msg
end

function frags(df, mf)
	dont = ""
	more = ""
	if df == 1 then
		dont = " (Don't Fragment)"
	end
	if mf == 1 then
		more = " (More Fragments)"
	end
	return dont, more
end

ipv4 = require("proto-ipv4")

return function(pkt)
	ip = ipv4(pkt)
	local good, bad, msg = checks(ip)
	local dont, more = frags(bool(ip.flags.df), bool(ip.flags.mf))

	print(string.format("Internet Protocol Version %d, Src: %s (%s), Dst: %s (%s)", ip.version, tostring(ip.src), tostring(ip.src), tostring(ip.dst), tostring(ip.dst)))
	print(string.format("    Version: %d", ip.version))
	print(string.format("    Header length: %d bytes", ip.hdr_len))
	print(string.format("    Total Length: %d", ip.len))
	print(string.format("    Identification: 0x%.04x (%d)", ip.id, ip.id))
	print(string.format("    Flags: 0x%.02x%s%s", ip.flags.all, dont, more))
	print(string.format("        %d... .... = Reserved bit: %s", bool(ip.flags.rb)))
	print(string.format("        .%d.. .... = Don't fragment: %s", bool(ip.flags.df)))
	print(string.format("        ..%d. .... = More fragments: %s", bool(ip.flags.mf)))
	print(string.format("    Fragment offset: %d", ip.frag_offset))
	print(string.format("    Time to live: %d", ip.ttl))
	print(string.format("    Protocol: %s (%d)", protocol(ip.proto), ip.proto))
	print(string.format("    Header checksum: 0x%04x %s", ip.checksum, msg))
	print(string.format("        %s", good))
	print(string.format("        %s", bad))
	print(string.format("    Source: %s (%s)", tostring(ip.src), tostring(ip.src)))
	print(string.format("    Destination: %s (%s)", tostring(ip.dst), tostring(ip.dst)))
	print()

	return packet.ACCEPT

end
