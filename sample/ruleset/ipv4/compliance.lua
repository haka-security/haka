------------------------------------
-- IP compliance
------------------------------------

haka.rule{
	hooks = { 'ipv4-up' },
	eval = function (self, pkt)
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
