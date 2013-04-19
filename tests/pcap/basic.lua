-- Basic test that will directly output the received packets.
-- The output should be the same as the input.

return function(pkt)
	return packet.ACCEPT
end
