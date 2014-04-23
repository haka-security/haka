------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
local tcp = require('protocol/tcp')

------------------------------------
-- Only allow packets to/from port 80
------------------------------------

haka.rule{
	-- The hooks tells where this rule is applied. Only TCP packets will be
	-- intecepted by this rule. Other protocol will flow.
	hook = tcp.events.receive_packet,
	eval = function (pkt)
		-- The next line will generate a lua error:
		-- there is no 'destport' field. replace 'destport' by 'dstport'
		if pkt.destport == 80 or pkt.srcport == 80 then
			haka.log("Filter", "Authorizing trafic on port 80")
		else
			haka.log("Filter", "Trafic not authorized on port %d", pkt.dstport)
			pkt:drop()
		end
	end
}
