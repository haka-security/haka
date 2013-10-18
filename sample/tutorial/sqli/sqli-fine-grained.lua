
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
	eval = function (self, pkt)
		dump_request(pkt)

		-- apply decoding functions on uri and cookie header
		local uri = decode_all(pkt.request.uri)
		local ck = decode_all(pkt.request.headers['Cookie'])

		-- initialize the score for query's argument and cookies list
		-- could be extend to check patterns in other http fields
		local where = {args = {score = 0}, cookies = {score = 0}}
		-- split query into list of (param-name, param-value) pairs
		where['args'].value = http.uri.split(uri).args
		-- split comma-separated cookies into a list of (key, value) pairs
		where['cookies'].value = http.cookies.split(ck)

		for _, key in ipairs(keywords) do
			for k, v in pairs(where) do
				-- loop on each query param | cookie value
				if v.value then
					for param, value in pairs(v.value) do
						if value:find(key) then
							v.score = v.score + 4
							if v.score >= 8 then
								haka.log.error("sqli", "    SQLi attack detected !!!")
								pkt:drop()
								return
							end
						end
					end
				end
			end
		end
	end
}

