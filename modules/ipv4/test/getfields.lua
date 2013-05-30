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

require("ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		local good, bad, msg = checks(pkt)
		local dont, more = frags(bool(pkt.flags.df), bool(pkt.flags.mf))

		print(string.format("Internet Protocol Version %d, Src: %s (%s), Dst: %s (%s)", pkt.version, pkt.src, pkt.src, pkt.dst, pkt.dst))
		print(string.format("    Version: %d", pkt.version))
		print(string.format("    Header length: %d bytes", pkt.hdr_len))
		print(string.format("    Total Length: %d", pkt.len))
		print(string.format("    Identification: 0x%.04x (%d)", pkt.id, pkt.id))
		print(string.format("    Flags: 0x%.02x%s%s", pkt.flags.all, dont, more))
		print(string.format("        %d... .... = Reserved bit: %s", bool(pkt.flags.rb)))
		print(string.format("        .%d.. .... = Don't fragment: %s", bool(pkt.flags.df)))
		print(string.format("        ..%d. .... = More fragments: %s", bool(pkt.flags.mf)))
		print(string.format("    Fragment offset: %d", pkt.frag_offset))
		print(string.format("    Time to live: %d", pkt.ttl))
		print(string.format("    Protocol: %s (%d)", protocol(pkt.proto), pkt.proto))
		print(string.format("    Header checksum: 0x%04x %s", pkt.checksum, msg))
		print(string.format("        %s", good))
		print(string.format("        %s", bad))
		print(string.format("    Source: %s (%s)", pkt.src, pkt.src))
		print(string.format("    Destination: %s (%s)", pkt.dst, pkt.dst))
		print()
	end
}
