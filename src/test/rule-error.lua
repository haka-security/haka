-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require("protocol/ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		pkt.none.crash = 1
	end
}
