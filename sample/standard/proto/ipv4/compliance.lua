------------------------------------
-- IP compliance
------------------------------------

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		-- bad IP checksum
		if not pkt:verify_checksum() then
			haka.log.error("filter", "Bad IP checksum")
			pkt:drop()
		end
	end
}
