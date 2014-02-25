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

		if current:available() > 30 then
			flow:reset()
		else
			while current:available() > 0 do
				local buf = haka.vbuffer(4)
				buf:sub():setfixedstring("Haka")

				current:insert(buf)
				current:advance(10)
			end
		end
	end
}
