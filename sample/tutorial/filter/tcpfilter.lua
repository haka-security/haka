------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')

------------------------------------
-- Security rule
------------------------------------

haka.rule {
	-- The hooks tells where this rule is applied
	-- Only TCP packets will be concerned by this rule
	-- Other protocol will flow
	hooks = {'tcp-up'},
		eval = function (self, pkt)
			-- We want to accept connection to or from
			-- TCP port 80 (request/response)
			if pkt.dstport == 80 or pkt.srcport == 80 then
				haka.log.info("Filter", "Authorizing trafic on port 80")
			else
				pkt:drop()
				haka.log.info("Filter", "This is not TCP port 80")
			end
        end
}
