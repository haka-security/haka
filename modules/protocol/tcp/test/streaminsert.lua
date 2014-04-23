-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
local tcp_connection = require("protocol/tcp-connection")

haka.rule {
	hook = tcp_connection.events.receive_data,
	eval = function (flow, data)
		haka.log.debug("filter", "received stream len=%d", #data)

		local current = data:pos('begin')
		while current:available() > 0 do
			local buf2 = haka.vbuffer_allocate(4)
			buf2[1] = 0x48
			buf2[2] = 0x61
			buf2[3] = 0x6b
			buf2[4] = 0x61

			current:insert(buf2)
			print(current:sub(10):asstring())
		end
	end
}
