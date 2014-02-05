-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, stream)
		local current = stream.current
		haka.log.debug("filter", "received stream len=%d", current:available())

		while current:available() > 0 do
			local buf2 = haka.vbuffer(4)
			buf2[1] = 0x48
			buf2[2] = 0x61
			buf2[3] = 0x6b
			buf2[4] = 0x61

			current:insert(buf2)
			print(current:sub(10):asstring())
		end
	end
}
