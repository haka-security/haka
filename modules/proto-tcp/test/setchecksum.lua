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

	--local good, bad = checks(ip_h)

	--output:write(string.format( "%d\n", tcp_h.checksum))
	tcp_h:computeChecksum()
	--output:write(string.format( "%d\n", tcp_h:computeChecksum()))
	--output:write(string.format( "%d\n", tcp_h.checksum))

	return haka.packet.ACCEPT

end
