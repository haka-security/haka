-- Basic test that will output some basic information about the
-- received packets.

ipv4 = require("proto-ipv4")
tcp = require("proto-tcp")

return function(pkt)

	local ip_h = ipv4(pkt)
	local tcp_h = tcp(ip_h)

	if tcp_h.dstport == 80 then
		return packet.ACCEPT
	else
		return packet.DENY
	end

end
