
require("ipv4")
require("tcp")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		-- filtering packet based on ip sources
		if pkt.src == ipv4.addr(127, 0, 0, 0) then
			haka.log("filter", "\tFiltering local destination addresses")
			pkt:drop()
		end
	end
}

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		-- checking IP checksum
		pkt:verify_checksum()
	end
}

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		-- printing TCP destination port
		haka.log.debug("filter", "\tTCP destination port: %d", pkt.dstport)
	end
}

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		-- setting destination port to 8080 (see out.pcap for changes)
		pkt.dstport = 8080
	end
}
