
require('httpconfig')

------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select', 'insert', 'update', 'delete', 'union'
}

------------------------------------
-- SQLi Naive Rule
------------------------------------

haka.rule {
	-- evaluation applies on upcoming requests
	hooks = { 'http-request' },
	eval = function (self, http)
		dump_request(http)

		local score = 0
		-- http fields (uri, headers) are available through 'request' field
		local uri = http.request.uri

		for _, key in ipairs(keywords) do
			-- check the whole uri against the list of malicious keywords
			if uri:find(key) then
				-- update the score
				score = score + 4
			end
		end
		
		if score >= 8 then
			-- raise an error if the score exceeds a fixed threshold 
			haka.log.error("sqli", "    SQLi attack detected with score %d", score)
			http:drop()
			return
		end
	end
}
