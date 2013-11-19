-- Basic test that will create a new packet from scratch
local ipv4 = require("protocol/ipv4")
require("protocol/icmp")

-- just to be safe, to avoid the test to run in an infinite loop
local counter = 10

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		if pkt.type ~= 6 then
			if counter == 0 then
				error("loop detected")
			end
			counter = counter-1

			local npkt = haka.dissector.get('raw'):create()
			npkt = haka.dissector.get('ipv4'):create(npkt)
			npkt.src = ipv4.addr(192, 168, 0, 1)
			npkt.dst = ipv4.addr("192.168.0.2")

			npkt = haka.dissector.get('icmp'):create(npkt)
			npkt.type = 6
			npkt.code = 1

			npkt:inject()
		end
	end
}
