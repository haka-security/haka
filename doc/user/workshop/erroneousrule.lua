-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
local tcp = require('protocol/tcp')

-- Allow only packets to/from port 80
haka.rule{
	on = haka.dissectors.tcp.events.receive_packet,
	eval = function (pkt)
		-- The next line will generate a lua error:
		-- there is no 'destport' field. Replace 'destport' by 'dstport'
		if pkt.destport == 80 or pkt.srcport == 80 then
			haka.log("Authorizing trafic on port 80")
		else
			haka.log("Trafic not authorized on port %d", pkt.dstport)
			pkt:drop()
		end
	end
}
