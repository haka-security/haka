-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- This test will check that tcp-rule hook
-- is functional. See rule-up test too.

require("protocol/ipv4")
require("protocol/tcp")

haka.rule {
	hooks = {"tcp-down"},
	eval = function (self,pkt)
		print(string.format("srcport:%s - dstport:%s", pkt.srcport, pkt.dstport))
	end
}
