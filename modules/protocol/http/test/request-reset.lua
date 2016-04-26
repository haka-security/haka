-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/http")

haka.rule {
	hook = haka.dissectors.http.events.request,
	eval = function(http, request)
		haka.log.debug("%s", request.uri)
		if #request.uri > 10 then
			http:reset()
		end
	end
}
