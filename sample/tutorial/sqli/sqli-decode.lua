require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select', 'insert', 'update', 'delete', 'union'
}

-- Still naive rule
haka.rule {
	hooks = { 'http-request' },
	eval = function (self, http)
		local score = 0
		local uri = http.request.uri
		-- apply all decoding functions on uri (percent-decode, uncomments, etc.)
		uri = decode_all(uri)
		for _, key in ipairs(keywords) do
			if uri:find(key) then
				score = score + 4
				if score >= 8 then
					haka.log.error("filter", "SQLi attack detected !!!")
					http:drop()
				end
			end
		end
	end
}

