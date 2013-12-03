-- This script is meant to be launched with a capture type nfqueue or pcap

-- Mandatory to load ipv4 dissector, Once dissector is loaded, we can access
-- all fields of IPv4 packets inside this script
local ipv4 = require("protocol/ipv4")

-- Mandatory to load tcp dissector, it also loads all the magic to track
-- connections
require("protocol/tcp")

-- For example, to reject bad checksums, we simply 
-- create a rule
haka.rule{
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		-- check for bad IP checksum
		if not pkt:verify_checksum() then
			-- raise an alert
			haka.alert{
				description = "Bad TCP checksum",
			}
			-- and drop the packet
			pkt:drop()
			-- Here we drop the packet, but we could also calculate and set
			-- the good checksum by calling the appropriate function
			-- (see documentation)
		end
	end
}

-- For example, we want to log something about connections
haka.rule{
	hooks = { "tcp-connection-new" },
	eval = function (self, pkt)
		local tcp = pkt.tcp
		if tcp.ip.dst == ipv4.addr("192.168.20.1") and tcp.dstport == 80 then
			haka.log.debug("filter","Traffic on HTTP port from %s", tcp.ip.src)
		end
	end
}
