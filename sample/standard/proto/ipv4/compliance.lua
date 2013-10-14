------------------------------------
-- IP compliance
------------------------------------

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (pkt)
		-- bad IP checksum
		if not pkt:verify_checksum() then
			haka.log.error("filter", "Bad IP checksum")
			pkt:drop()
		end
	end
}
