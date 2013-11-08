
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
	-- Evaluation applies on upcoming requests
	hooks = { 'http-request' },
	eval = function (self, http)
		dump_request(http)

		local score = 0
		-- Http fields (uri, headers) are available through 'request' field
		local uri = http.request.uri

		for _, key in ipairs(keywords) do
			-- Check the whole uri against the list of malicious keywords
			if uri:find(key) then
				-- Update the score
				score = score + 4
			end
		end
		
		if score >= 8 then
			-- Raise an error if the score exceeds a fixed threshold 
			haka.log.error("sqli", "    SQLi attack detected with score %d", score)
			http:drop()
			return
		end
	end
}
