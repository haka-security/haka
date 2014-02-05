-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, stream)
		local buf = haka.vbuffer(4)
		buf:setfixedstring("Haka")
		stream.current:insert(buf)
	end
}

haka.rule {
	hook = haka.event('tcp-connection', 'send_data'),
	eval = function (flow, stream)
		stream.current:erase(10)
	end
}
