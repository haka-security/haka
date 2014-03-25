-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = haka.event('http', 'response_data'),
	streamed = true,
	eval = function (http, iter)
		print("== RESPONSE DATA ==")
		while true do
			local sub = iter:sub('available')
			if not sub then break end

			print(safe_string(sub:asstring()))
		end
	end
}
