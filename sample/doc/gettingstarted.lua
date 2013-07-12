-- This script is meant to be launched with a capture type
-- nfq, or pcap, as:
-- hake capture-nfq.lua eth0:eth1 gettingstarted.lua
--

-- mandatory to load ipv4 dissector
require("ipv4")
-- Once dissector is loaded, we can access all fields
-- of IPv4 packets inside this script.

-- For example, to reject bad checksums, we simply 
-- create a rule
haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
	  -- bad IP checksum
	  if not pkt:verify_checksum() then
		-- Logging type is described in capture-type file
		haka.log.error("filter", "Bad IP checksum")
		pkt:drop() 
		-- Here we drop the packet, but we could
		-- also calculate and set the good checksum
		-- by calling the appropriate function
		-- (see docs)
	  end
	end
  }

-- mandatory to load tcp dissector
-- It also loads all the magic to track connection
require("tcp")

-- For exemple, we want to log somethings about
-- connections
akpf:rule {
    hooks = {"tcp-connection-new"},
    eval = function (self, pkt)
		if tcp.ip.dst == ipv4.addr("192.168.1.1") and tcp.dstport == 80 then
			haka.log.warning("filter","Trafic on HTTP port from %s", tcp.ip.src)
		end
	end
	}

-- For more example and possibilities, read this doc,
-- Chapters about rule, rule group and dissector.
