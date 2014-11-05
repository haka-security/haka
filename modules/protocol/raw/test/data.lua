-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will test raw packet

local raw = require("protocol/raw")

haka.rule {
	hook = raw.events.receive_packet,
	eval = function (pkt)
		pkt.data = "foo"
		print(pkt.data)
	end
}
