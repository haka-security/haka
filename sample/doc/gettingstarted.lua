-- This script is meant to be launched with a capture type
-- nfq, or pcap, as:
-- haka capture-nfq.lua eth0:eth1 gettingstarted.lua

-- Mandatory to load ipv4 dissector
local ipv4 = require("protocol/ipv4")
-- Once dissector is loaded, we can access all fields
-- of IPv4 packets inside this script.

-- Mandatory to load tcp dissector
-- It also loads all the magic to track connections
require("protocol/tcp")

-- For example, to reject bad checksums, we simply 
-- create a rule
haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
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
	hooks = {"tcp-connection-new"},
	eval = function (self, pkt)
		if pkt.tcp.ip.dst == ipv4.addr("192.168.20.1") and pkt.tcp.dstport == 80 then
			haka.log.warning("filter","Trafic on HTTP port from %s", pkt.tcp.ip.src)
		end
	end
}

-- For more example and possibilities, read this doc,
-- Chapters about rule, rule group and dissector.
