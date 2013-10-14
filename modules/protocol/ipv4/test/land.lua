
require('protocol/ipv4')

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (pkt)
		if pkt.src == pkt.dst then
			pkt:drop()
		end
	end
}
