
require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select', 'insert', 'update', 'delete', 'union'
}

------------------------------------
-- A Better Naive Rule ...
------------------------------------

haka.rule {
	hooks = { 'http-request' },
	eval = function (self, http)
		dump_request(http)

		-- apply decoding functions on uri and cookie header
		local uri = decode_all(http.request.uri)
		local ck = decode_all(http.request.headers['Cookie'])

		-- initialize the score for query's argument and cookies list
		-- could be extend to check patterns in other http fields
		local where = {
			args = {
				-- split query into list of (param-name, param-value) pairs
				value = httplib.uri.split(uri).args,
				score = 0
			},
			cookies = {
				-- split comma-separated cookies into a list of (key, value)
				-- pairs
				value = httplib.cookies.split(ck),
				score = 0
			}
		}

		for k, v in pairs(where) do
			if v.value then
				for _, key in ipairs(keywords) do
				-- loop on each query param | cookie value
					for param, value in pairs(v.value) do
						if value:find(key) then
							v.score = v.score + 4
						end
					end
				end
			end

			if v.score >= 8 then
				haka.log.error("sqli", "    SQLi attack detected in %s with score %d", k, v.score)
				http:drop()
				return
			end
		end
	end
}

