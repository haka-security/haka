require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select', 'insert', 'update', 'delete', 'union'
}

-- Still naive rule
haka.rule{
	hook = httplib.events.request,
	eval = function (http, request)
		dump_request(request)

		local score = 0
		local uri = request.uri

		-- Apply all decoding functions on uri (percent-decode, uncomments, etc.)
		uri = decode_all(uri)
		for _, key in ipairs(keywords) do
			if uri:find(key) then
				score = score + 4
			end
		end

		if score >= 8 then
			haka.alert{
				description = string.format("SQLi attack detected with score %d", score),
				severity = 'high',
				confidence = 'low',
			}
			http:drop()
		end
	end
}
