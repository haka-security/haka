------------------------------------
-- Firewall rules
------------------------------------

local client_network = ipv4.network("192.168.10.0/25");
local server_network = ipv4.network("192.168.20.0/25");

local group = haka.rule_group {
	hook = tcp_connection.events.new_connection,
	init = function (flow, pkt)
		haka.log.debug("filter", "entering packet filtering rules : %d --> %d",
			pkt.srcport, pkt.dstport)
	end,
	final = function (flow, pkt)
		haka.alert{
			description = "Packet dropped : drop by default",
			sources = haka.alert.address(pkt.ip.src, pkt.srcport),
			targets = haka.alert.address(pkt.ip.dst, pkt.dstport)
		}
		pkt:drop()
	end,
	continue = function (ret, flow, pkt)
		return not ret
	end
}


group:rule(
	function (flow, tcp)
		if client_network:contains(tcp.ip.src) and
		    server_network:contains(tcp.ip.dst) and
		    tcp.dstport == 80 then
			haka.log.warning("filter", "authorizing http traffic")
			flow:select_next_dissector(haka.dissector.get('http'):new(flow))
			return true
		end
	end
)

group:rule(
	function (flow, tcp)
		if client_network:contains(tcp.ip.src) and
		    server_network:contains(tcp.ip.dst) and
		    tcp.dstport == 22 then
			haka.log.warning("filter", "authorizing ssh traffic")
			haka.log.warning("filter", "no available dissector for ssh")
			return true
		end
	end
)
