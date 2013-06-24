------------------------------------
-- IP attacks
------------------------------------

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if pkt.src == pkt.dst then
			haka.log.error("filter", "Land attack detected")
			pkt:drop()
		end
	end
}
