-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/tcp_connection")
require("protocol/http")

haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		print("HTTP REQUEST")
		debug.pprint(request, nil, nil, { debug.hide_underscore, debug.hide_function })
	end
}

haka.rule {
	on = haka.dissectors.http.events.response,
	eval = function (http, response)
		print("HTTP RESPONSE")
		debug.pprint(response, nil, nil, { debug.hide_underscore, debug.hide_function })
	end
}
