-- Basic test that will output some basic information about the
-- received packets.

return function(pkt)
	print(string.format("Received packet of %d bytes", pkt.length))
	return haka.packet.ACCEPT
end
