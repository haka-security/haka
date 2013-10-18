
require('httpconfig')

------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select','insert','update','delete', 'union'
}

------------------------------------
-- SQLi Naive Rule
------------------------------------

haka.rule {
	-- evalution applies on upcoming requests
	hooks = { 'http-request' },
	eval = function (self, http)
		dump_request(http)

		local score = 0
		-- http fileds (uri, headers) are available through 'request' field
		local uri = http.request.uri
		for _, key in ipairs(keywords) do
			-- check the whole uri against the list of malicious keywords
			if uri:find(key) then
				-- update the score and raise an error if the score exceeds a threshold
				score = score + 4
				if score >= 8 then
					haka.log.error("sqli", "    SQLi attack detected !!!")
					http:drop()
				end
			end
		end
	end
}
