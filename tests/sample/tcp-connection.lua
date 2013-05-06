
app.install("packet", module.load("packet-pcap", {"-i", "lo"}))
app.install("log", module.load("log-stdout"))

require("proto-ipv4")
require("proto-tcp")

app.install_filter(function (pkt)

	local ip_h = ipv4(pkt)
	local tcp_h = tcp(ip_h)
	local conn

	if (tcp_h) then
		conn = tcp_h:getConnection()
		if (not conn) then
			-- new connection
			if (tcp_h.flags.syn) then
				conn = tcp_h:newConnection()
				log.debug("filter", "new connection %s (%d) --> %s (%d)", tostring(conn.srcip), conn.srcport, tostring(conn.dstip), conn.dstport)
			-- packet do not belong to existing connection
			else
				return packet.DROP
			end

		else
			-- end existing connection
			if (tcp_h.flags.fin) or (tcp_h.flags.rst) then
				log.debug("filter", "ending connection %s (%d) --> %s (%d)", tostring(conn.srcip), conn.srcport, tostring(conn.dstip), conn.dstport)

				conn:close()
			end
		end
	end
	return packet.ACCEPT
end)

log.info("config", "Done\n")
