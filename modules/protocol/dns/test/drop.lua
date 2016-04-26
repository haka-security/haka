-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/dns")

haka.rule {
	hook = haka.dissectors.dns.events.query,
	eval = function (dns, query)
		if query.id == 60714 then
			query:drop()
		end
	end
}

haka.rule {
	hook = haka.dissectors.dns.events.query,
	eval = function (dns, query)
		print("DNS QUERY")
		debug.pprint(query, nil, nil, { debug.hide_underscore, debug.hide_function })
	end
}
