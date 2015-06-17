-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require("protocol/ipv4");

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		local addr = ipv4.addr("192.168.10.0");
		local net10 = ipv4.network(addr, 24);
		local net20 = ipv4.network("192.168.20.0/24");

		if net10:contains(pkt.src) then
			haka.log.debug("net=%s/%s ; host=%s", net10.net, net10.mask, pkt.src);
		end
		if net20:contains(pkt.src) then
			haka.log.debug("net=%s/%s ; host=%s", net20.net, net20.mask, pkt.src);
		end
	end
}
