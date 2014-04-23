
-- load the IPV4 disector to be able to read the fields of ipv4 packets
local ipv4 = require("protocol/ipv4")

-- load the tcp disector, this is needed to be able to track connections
local tcp = require("protocol/tcp")
local tcp_connection = require("protocol/tcp-connection")

-- rule to check packet for bad TCP checksums and reject them
haka.rule{
	hook = tcp.events.receive_packet, -- hook on tcp packets, before any sub-protocol is parsed.
	eval = function (pkt)
		-- check for bad IP checksum
		if not pkt:verify_checksum() then
			-- raise an alert
			haka.alert{
				description = "Bad TCP checksum",
			}
			-- and drop the packet
			pkt:drop()
			-- alternatively, set the correct checksum
			--[[
			pkt:compute_checksum()
			--]]
		end
	end
}

-- rule to add a log entry on HTTP connections to a web server
haka.rule{
	hook = tcp_connection.events.new_connection, --hook on new TCP connections.
	eval = function (flow, tcp)
		if tcp.ip.dst == ipv4.addr("192.168.20.1") and tcp.dstport == 80 then
			haka.log.debug("filter","Traffic on HTTP port from %s", tcp.ip.src)
		end
	end
}
