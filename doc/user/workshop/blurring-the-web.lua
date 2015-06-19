-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local http = require('protocol/http')
local rem = require('regexp/pcre')

http.install_tcp_rule(80)

local pattern = '</head>'
local regexp = rem.re:compile(pattern, rem.re.CASE_INSENSITIVE)
local css = ' <style type="text/css" media="screen"> * { color: transparent !important; text-shadow: 0 0 3px black !important; } </style> '

haka.rule {
	hook = http.events.request,
	eval = function (http, request)
		request.headers['Accept-Encoding'] = nil
		request.headers['Accept'] = '*/*'
		request.headers['If-Modified-Since'] = nil
		request.headers['If-None-Match'] = nil

		haka.log("received request for %s on %s", request.uri,
			request.headers['Host'])
	end
}

haka.rule {
	hook = http.events.response,
	eval = function (http, response)
		http:enable_data_modification()
	end
}

haka.rule{
	hook = http.events.response_data,
	options = {
		streamed = true,
	},
	eval = function (flow, iter)
		local result = regexp:match(iter)
		if result then
			haka.log("bluring response")
			iter:insert(haka.vbuffer_from(css))
		end
	end
}
