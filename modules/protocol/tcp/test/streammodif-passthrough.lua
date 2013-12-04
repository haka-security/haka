-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")

local buf = haka.buffer(4)
buf[1] = 0x48
buf[2] = 0x61
buf[3] = 0x6b
buf[4] = 0x61

haka.rule {
	hooks = { "tcp-connection-up" },
	eval = function (self, pkt)
		pkt.stream:insert(buf)
	end
}

haka.rule {
	hooks = { "tcp-connection-down" },
	eval = function (self, pkt)
		pkt.stream:erase(10)
	end
}
