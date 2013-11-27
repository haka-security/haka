-- Basic test that will output ip options (if present)

require("protocol/ipv4")

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (pkt)
		local opt = pkt.opt
		if opt then
			print(string.format("Packet from %s to %s with %d IP options\n", pkt.src, pkt.dst, #opt))
			for iter = 1, #opt do
				print(string.format("    Version: %d", opt[iter].copy))
				print(string.format("    Class:   %d", opt[iter].class))
				print(string.format("    Number:  %d", opt[iter].number))
				if opt[iter].len then
					print(string.format("    Length:  %d", opt[iter].len))
				end
				print()
			end
			print()
		end
	end
}
