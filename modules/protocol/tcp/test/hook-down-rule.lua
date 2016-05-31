-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- This test will check that tcp-rule hook
-- is functional. See rule-up test too.

require("protocol/ipv4")
local tcp = require("protocol/tcp")
require("protocol/tcp_connection")

haka.rule {
	on = haka.dissectors.tcp.events.send_packet,
	eval = function (pkt)
		print(string.format("srcport:%s - dstport:%s", pkt.srcport, pkt.dstport))
	end
}
