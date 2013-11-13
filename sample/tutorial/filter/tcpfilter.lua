------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
require('protocol/tcp')

------------------------------------
-- Security rule
------------------------------------

haka.rule {
	-- The hooks tells where this rule is applied.
	-- Only TCP packets will be concerned by this rule.
	-- Other protocol will flow
	hooks = { 'tcp-up' },
		eval = function (self, pkt)
			-- The next line will generate a lua error:
			-- there is no 'destport' field. replace 'destport'
			-- by 'dstport'.
			if pkt.destport == 80 or pkt.srcport == 80 then
				haka.log("Filter", "Authorizing trafic on port 80")
			else
				haka.log("Filter", "Trafic not authorized on port %d", pkt.dstport)
				pkt:drop()
			end
        end
}
