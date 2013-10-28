--The require lines tolds Haka which dissector
--it has to register. Those dissectors gives hook
--to interfere with it.
require("protocol/ipv4")
require("protocol/tcp")

--This is a rule
haka.rule {
        --The hooks tells where this rule is applied
        --Only TCP packet will be concerned by this rule
        --Other protocol will flow
        hooks = {"tcp-up"},
        --Each rule have an eval function
        --Eval function is always build with (self, name)
                --First parameter, self, is mandatory
                --Second parameter can be named whatever you want
        eval = function (self, pkt)
                --All fields is accessible through accessors
                --following wireshark/tcpdump semantics
                --Documentation give the complet list of accessors
				--You can choose to let the packet flow, or block it.
				if pkt.dstport == 80 or pkt.srcport == 80 then
					-- We want to accept this connexion
					-- It's either a request to a website or a response
					haka.log.info("Filter", "Authorizing trafic on port 80")
				else
					pkt:drop()
					haka.log.info("Filter", "This is not port 80")
				end
        end
}
