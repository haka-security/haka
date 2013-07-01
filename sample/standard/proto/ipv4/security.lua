------------------------------------
-- IP attacks
------------------------------------

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if (pkt.src == pkt.dst) and (pkt.src ~= ipv4.addr("127.0.0.1")) then
			haka.log.error("filter", "Land attack detected")
			pkt:drop()
		end
	end
}
