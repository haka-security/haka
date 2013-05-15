-- Basic test that will output some basic information about the
-- received packets.

ipv4 = require("proto-ipv4")
tcp = require("proto-tcp")

return function(pkt)

	local ip_h = ipv4(pkt)
	if ip_h.proto == 6 then
		local tcp_h = tcp(ip_h)
		local conn

		if (tcp_h) then
			conn = tcp_h:getConnection()
			if (not conn) then
				-- new connection
				if (tcp_h.flags.syn) then
					conn = tcp_h:newConnection()
					print(string.format("new connection %s (%d) --> %s (%d)", tostring(conn.srcip), conn.srcport, tostring(conn.dstip), conn.dstport))
				else
					-- packet do not belong to existing connection
					print(string.format("unknown connection %s (%d) --> %s (%d)", tostring(ip_h.src), tcp_h.srcport, tostring(ip_h.dst), tcp_h.dstport))
					return haka.packet.DROP
				end
																																	
			else
			-- end existing connection	
				if (tcp_h.flags.fin) or (tcp_h.flags.rst) then
					print(string.format("ending connection %s (%d) --> %s (%d)", tostring(conn.srcip), conn.srcport, tostring(conn.dstip), conn.dstport))
					conn:close()
				end
			end
		end
	end
	return haka.packet.ACCEPT

end
