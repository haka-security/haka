-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")

haka.rule {
	hooks = { "tcp-connection-up" },
	eval = function (self, pkt)
		haka.log.debug("filter", "received stream len=%d", pkt.stream:available())

		if pkt.stream:available() > 10 then
			pkt:reset()
		end
	end
}
