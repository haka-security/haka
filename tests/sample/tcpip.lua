-- loading pcap module
app.install("packet", module.load("packet-pcap", {"-f", "http.pcap", "-o", "out.pcap"}))
-- loading log module
app.install("log", module.load("log-stdout"))
    
require("proto-ipv4")
require("proto-tcp")
    
app.install_filter(function (pkt)

	local ip_h = ipv4(pkt)
	local tcp_h = tcp(ip_h)
    
    -- filtering packet based on ip sources
    if ip_h.src == ipv4.addr(127, 0, 0, 0) then
		log("filter", "\tFiltering local destination addresses")
		return packet.DROP
    end
    
    -- checking IP checksum
    ip_h:verifyChecksum()

	-- printing TCP destination port
    log.debug("filter", "\tTCP destination port: %d", tcp_h.dstport)

    -- setting destination port to 8080 (see out.pcap for changes)
    tcp_h.dstport = 8080
    
    return packet.ACCEPT
end)

