------------------------------------
-- IP attacks
------------------------------------

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if (pkt.src == pkt.dst) and (pkt.src ~= ipv4.addr("127.0.0.1")) then
			haka.alert{
				description = "Land attack detected",
				severity = 'low',
				confidence = 'medium',
				sources = { haka.alert.address(pkt.src) },
			}
			pkt:drop()
		end
	end
}
