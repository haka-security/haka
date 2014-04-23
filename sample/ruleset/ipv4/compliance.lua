------------------------------------
-- IP compliance
------------------------------------

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		-- bad IP checksum
		if not pkt:verify_checksum() then
			haka.alert{
				description = "Bad IP checksum",
				sources = { haka.alert.address(pkt.src) },
				targets = { haka.alert.address(pkt.dst) },
			}
			pkt:drop()
		end
	end
}
