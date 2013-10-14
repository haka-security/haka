------------------------------------
-- IP attacks
------------------------------------

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (self, pkt)
		if (pkt.src == pkt.dst) and (pkt.src ~= ipv4.addr("127.0.0.1")) then
			haka.log.error("filter", "Land attack detected")
			pkt:drop()
		end
	end
}
