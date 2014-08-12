-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
local tcp_connection = require("protocol/tcp_connection")

haka.rule {
	hook = tcp_connection.events.receive_data,
	eval = function (flow, data)
		data:pos('begin'):sub(10, true):erase()
	end
}
