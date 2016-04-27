-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require('protocol/ipv4')

haka.rule {
	on = haka.dissectors.ipv4.events.receive_packet,
	eval = function (pkt)
		local myalert = haka.alert{
			start_time = pkt.raw.timestamp,
			end_time = pkt.raw.timestamp,
			description = string.format("filtering IP %s", pkt.src),
			severity = 'medium',
			confidence = 7,
			completion = 'failed',
			method = {
				description = "packet sent on the network",
				ref = { "cve/255-45", "http://...", "cwe:dff" }
			},
			sources = { haka.alert.address(pkt.src, "local.org", "blalvla", 33), haka.alert.service(22, "ssh") },
			targets = { haka.alert.address(ipv4.network(pkt.dst, 22)), haka.alert.address(pkt.dst) },
		}

		myalert:update{
			severity = 'high',
			description = string.format("filtering IP %s", pkt.src),
			confidence = 'low',
			method = {
				ref = "cve/255-45"
			},
			sources = haka.alert.address(pkt.src),
			ref = myalert
		}
	end
}
