-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require('protocol/ipv4')

local testPolicy = haka.policy.new("test_policy")

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		testPolicy:apply{
			values = {
				ip_src = pkt.src,
				ip_dst = pkt.dst
			},
			desc = {
				start_time = pkt.raw.timestamp,
				end_time = pkt.raw.timestamp,
				sources = { haka.alert.address(pkt.src) },
				targets = { haka.alert.address(pkt.dst) },
			},
			ctx = pkt
		}
	end
}

haka.policy {
	on = testPolicy,
	name = "test that a policy can be defined and used",
	ip_src = ipv4.addr('192.168.20.99'),
	ip_dst = ipv4.addr('192.168.20.1'),
	action = haka.policy.drop_with_alert{
		description = "forbidden traffic to dmz",
	},
}
