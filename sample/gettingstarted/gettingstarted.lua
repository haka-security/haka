
-- load the ipv4 dissector to be able to read the fields of ipv4 packets
local ipv4 = require("protocol/ipv4")

-- load the tcp dissectors (staefull and stateless)
-- this is needed to be able to track tcp connections
-- and to get access to tcp packet fields
local tcp = require("protocol/tcp")
local tcp_connection = require("protocol/tcp_connection")

-- security rule to discard packets with bad tcp checksums
haka.rule{
	hook = tcp.events.receive_packet, -- hook on new tcp packets capture
	eval = function (pkt)
		-- check for bad ip checksum
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

-- securty rule to add a log entry on http connections to a web server
haka.rule{
	hook = tcp_connection.events.new_connection, --hook on new tcp connections.
	eval = function (flow, tcp)
		local web_server = ipv4.addr("192.168.20.1")
		if tcp.ip.dst == web_server and tcp.dstport == 80 then
			haka.log.debug("filter","Traffic on HTTP port from %s", tcp.ip.src)
		end
	end
}
