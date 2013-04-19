-- Basic test that will output some basic information about the
-- received packets.

return function(pkt)
	output:write(string.format("Received packet of %d bytes\n", pkt.length))
	return packet.ACCEPT
end
