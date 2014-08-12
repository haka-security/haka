-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
local tcp_connection = require("protocol/tcp_connection")

haka.rule {
	hook = tcp_connection.events.receive_data,
	options = {
		streamed = true,
	},
	eval = function (flow, iter, direction)
		local sub
		local data = {}

		for sub in iter:foreach_available() do
			haka.log("test", "connection=%s:%d->%s:%d, dir %s, data received len=%d ",
				flow.srcip, flow.srcport,
				flow.dstip, flow.dstport,
				direction, #sub)

			table.insert(data, sub:asstring())
		end

		haka.log("test", "connection=%s:%d->%s:%d, dir %s, data: %s",
				flow.srcip, flow.srcport,
				flow.dstip, flow.dstport,
				direction, table.concat(data))
	end
}
