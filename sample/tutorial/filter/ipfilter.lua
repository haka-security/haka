--The require lines tolds Haka which dissector
--it has to register. Those dissectors gives hook
--to interfere with it.
require("protocol/ipv4")

--This is a rule
haka.rule {
        --The hooks tells where this rule is applied
        hooks = {"ipv4-up"},
        --Each rule have an eval function
        --Eval function is always build with (self, name)
                --First parameter, self, is mandatory
                --Second parameter can be named whatever you want
        eval = function (self, pkt)
                --All fields is accessible through accessors
                --following wireshark/tcpdump semantics
                --Documentation give the complet list of accessors
				--You can choose to let the packet flow, or block it.
				if ip.src == ipv4addr("192.168.10.10") then
					-- We want to block this IP
					ip:drop()
					haka.log.info("Filter","Filtering IP 192.168.10.10")
				end
				if ip.src == ipv4addr("192.168.10.1") then
					-- We want to authorize this IP
					ip:accept()
				end
        end
}
