-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, data)
		haka.log.debug("filter", "received stream len=%d", #data)

		local current = data:pos('begin')
		while current:available() > 0 do
			current:sub(10, true):erase()
			print(current:sub(10):asstring())
		end
	end
}
