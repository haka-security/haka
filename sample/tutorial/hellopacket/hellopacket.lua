-- Use this file with hakapcap tool:
--
-- hakapcap hellopacket.pcap hellopacket.lua
-- (adjust paths)


--The require lines tolds Haka which dissector
--it has to register. Those dissectors gives hook
--to interfere with it.
require("protocol/ipv4")
require("protocol/tcp")

--This is a rule
haka.rule {
	--The hooks tells where this rule is applied
	hooks = {"ipv4-up"},
	--Each rule have an eval function
	--Function is always build with (self, name)
	--The name is later use for action
	eval = function (self, pkt)
		--All fields is accessible through accessors
		--following wireshark/tcpdump semantics
		--Documentation give the complet list of accessors
		haka.log("debug","Hello packet from %s to %s", pkt.src, pkt.dst)
	end
}

--Any number of rule is authorized
haka.rule {
	--The hooks is place on the TCP connection
	hooks = {"tcp-connection-new"},
	eval = function (self, pkt)
		--Fields from previous layer is accessible too
		haka.log("debug","Hello TCP connection from %s:%d to %s:%d", pkt.tcp.ip.src, pkt.tcp.srcport, pkt.tcp.ip.dst, pkt.tcp.dstport)
	end
}
