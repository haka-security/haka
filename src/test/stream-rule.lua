-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	coroutine = true,
	eval = function (flow, iter, direction)
		local sub
		local data = {}

		while true do
			sub = iter:sub('available')
			if not sub then break end

			haka.log("test", "connection=%s:%d->%s:%d, dir %s, data received len=%d ",
				flow.connection.srcip, flow.connection.srcport,
				flow.connection.dstip, flow.connection.dstport,
				direction, #sub)
			
			table.insert(data, sub:asstring())
		end

		haka.log("test", "connection=%s:%d->%s:%d, dir %s, data: %s",
				flow.connection.srcip, flow.connection.srcport,
				flow.connection.dstip, flow.connection.dstport,
				direction, table.concat(data))
	end
}
