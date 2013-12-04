-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, stream)
		haka.log.debug("filter", "received stream len=%d", stream:available())

		if stream:available() > 30 then
			flow:reset()
		else
			local buf2 = haka.buffer(4)
			buf2[1] = 0x48
			buf2[2] = 0x61
			buf2[3] = 0x6b
			buf2[4] = 0x61

			while stream:available() > 0 do
				stream:insert(buf2)
				local buf = stream:read(10)
			end
		end
	end
}
