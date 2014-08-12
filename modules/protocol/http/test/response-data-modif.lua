-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = http.events.response,
	eval = function (http, response)
		http:enable_data_modification()
	end
}

haka.rule {
	hook = http.events.response_data,
	options = {
		streamed = true,
	},
	eval = function (http, iter)
		print("== RESPONSE DATA ==", http.request.uri)
		iter:advance(20)
		iter:insert(haka.vbuffer_from("HAKA HAKA HAKA"))
	end
}
