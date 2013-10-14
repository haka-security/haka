-- This script is meant to be launched with a capture type nfq or pcap

-- Mandatory to load ipv4 dissector
local ipv4 = require("protocol/ipv4")
-- Once dissector is loaded, we can access all fields
-- of IPv4 packets inside this script.

-- Mandatory to load tcp dissector
-- It also loads all the magic to track connections
require("protocol/tcp")
require("protocol/tcp-connection")

-- For example, to reject bad checksums, we simply 
-- create a rule
haka.rule {
	hook = haka.event('tcp', 'receive_packet'),
	eval = function (pkt)
		-- bad IP checksum
		if not pkt:verify_checksum() then
			-- Logging type is described in capture-type file
			haka.log.error("filter", "Bad TCP checksum")
			pkt:drop() 
			-- Here we drop the packet, but we could
			-- also calculate and set the good checksum
			-- by calling the appropriate function
			-- (see docs)
		end
	end
}

-- For example, we want to log something about
-- connections
haka.rule {
	hook = haka.event('tcp-connection', 'new_connection'),
	eval = function (pkt)
		if pkt.ip.dst == ipv4.addr("192.168.20.1") and pkt.dstport == 80 then
			haka.log.warning("filter","Traffic on HTTP port from %s", pkt.ip.src)
		end
	end
}

-- For more example and possibilities, read this doc,
-- Chapters about rule, rule group and dissector.
