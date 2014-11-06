-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Basic test that will test raw packet

local raw = require("protocol/raw")

haka.rule {
	hook = raw.events.receive_packet,
	eval = function (pkt)
		if pkt then
			print("packet {")
			io.write("  data : ")
			debug.pprint(pkt.data, nil, nil, { debug.hide_underscore, debug.hide_function })
			print("  id : "..pkt.id)
			io.write("  payload : ")
			debug.pprint(pkt.payload, nil, nil, { debug.hide_underscore, debug.hide_function })
			print("  timestamp : "..tostring(pkt.timestamp))
			print("}")
		else
			print("nil packet")
		end
	end
}
