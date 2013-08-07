------------------------------------
-- Firewall rules
------------------------------------

local group = haka.rule_group {
	name = "group",
	init = function (self, pkt)
		haka.log.debug("filter", "entering packet filetring rules : %d --> %d", pkt.tcp.srcport, pkt.tcp.dstport)
	end,
	fini = function (self, pkt)
		haka.log.error("filter", "packet dropped : drop by default")
		pkt:drop()
	end,
	continue = function (self, pkt, ret)
		return not ret
	end
}


group:rule {
	hooks = {"tcp-connection-new"},
	eval = function (self, pkt)

		local netsrc = ipv4.network("10.2.96.0/22")
		local netdst = ipv4.network("10.2.104.0/22")

		local tcp = pkt.tcp

		if netsrc:contains(tcp.ip.src) and
		   netdst:contains(tcp.ip.dst) and
		   tcp.dstport == 80 then

			haka.log.warning("filter", "authorizing http traffic")
			pkt.next_dissector = "http"
			return true
		end
	end
}

group:rule {
	hooks = {"tcp-connection-new"},
	eval = function (self, pkt)

		local netsrc = ipv4.network("10.2.96.0/22")
		local netdst = ipv4.network("10.2.104.0/22")

		local tcp = pkt.tcp

		if netsrc:contains(tcp.ip.src) and
		   netdst:contains(tcp.ip.dst) and
		   tcp.dstport == 22 then

			haka.log.warning("filter", "authorizing ssh traffic")
			haka.log.warning("filter", "no available dissector for ssh")
			return true
		end
	end
}
