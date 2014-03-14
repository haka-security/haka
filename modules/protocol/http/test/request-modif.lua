-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		print("HTTP REQUEST")
		request:dump()
		-- We change a part of the request
		request.version = "2.0"
		-- We change an existing header
		request.headers["Host"] = "haka.powered.tld"
		-- We destroy one
		request.headers["User-Agent"] = nil
		-- and we create a new one
		request.headers["Haka"] = "Done"

		print("HTTP MODIFIED REQUEST")
		request:dump()
	end
}

haka.rule {
	hook = haka.event('http', 'response'),
	eval = function (http, response)
		print("HTTP RESPONSE")
		response:dump()
	end
}
