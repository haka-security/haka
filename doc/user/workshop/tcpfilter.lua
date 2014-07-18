-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

------------------------------------
-- Loading dissectors
------------------------------------

local tcp = require('protocol/tcp_connection')

-- Allow only packets to port 22 and 80
haka.rule{
	hook = tcp.events.new_connection,
	eval = function (flow, pkt)
		if flow.dstport == 22 or flow.dstport == 80 then
			haka.log("Filter", "Authorizing trafic on port %d", flow.dstport)
		else
			haka.log("Filter", "Trafic not authorized on port %d", flow.dstport)
			pkt:drop()
		end
	end
}
