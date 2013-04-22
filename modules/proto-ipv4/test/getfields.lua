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

	output:write(string.format("Internet Protocol Version %d, Src: %s (%s), Dst: %s (%s)\n", ip.version, tostring(ip.src), tostring(ip.src), tostring(ip.dst), tostring(ip.dst)))
	output:write(string.format("    Version: %d\n", ip.version))
	output:write(string.format("    Header length: %d bytes\n", ip.hdr_len))
    output:write(string.format("    Total Length: %d\n", ip.len))
    output:write(string.format("    Identification: 0x%.04x (%d)\n", ip.id, ip.id))
    output:write(string.format("    Flags: 0x%.02x%s%s\n", ip.flags.all, dont, more))
    output:write(string.format("        %d... .... = Reserved bit: %s\n", bool(ip.flags.rb)))
    output:write(string.format("        .%d.. .... = Don't fragment: %s\n", bool(ip.flags.df)))
    output:write(string.format("        ..%d. .... = More fragments: %s\n", bool(ip.flags.mf)))
	output:write(string.format("    Fragment offset: %d\n", ip.frag_offset))
	output:write(string.format("    Time to live: %d\n", ip.ttl))
	output:write(string.format("    Protocol: %s (%d)\n", protocol(ip.proto), ip.proto))
	output:write(string.format("    Header checksum: 0x%04x %s\n", ip.checksum, msg))
	output:write(string.format("        %s\n", good))
	output:write(string.format("        %s\n", bad))
	output:write(string.format("    Source: %s (%s)\n", tostring(ip.src), tostring(ip.src)))
	output:write(string.format("    Destination: %s (%s)\n\n", tostring(ip.dst), tostring(ip.dst)))

	return packet.ACCEPT

end

