-- Basic test that will output some basic information about the
-- received packets.

ipv4 = require("proto-ipv4")
tcp = require("proto-tcp")

return function(pkt)

	local ip = ipv4(pkt)
	if ip.proto == 6 then
		local tcp_h = tcp(ip)
		--Xmas scan, as made by nmap -sX <IP>
		if tcp_h.flags.psh and tcp_h.flags.fin and tcp_h.flags.urg then
			print "Xmas attack!! Details : "
			print(string.format("\tIP src: " .. tostring(ip.src) .. "; port src: " .. tcp_h.srcport))
			print(string.format("\tIP dst: " .. tostring(ip.dst) .. "; port dst: " .. tcp_h.dstport))
		end
	end
	return packet.ACCEPT

end
