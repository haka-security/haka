
require('httpconfig')

------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select','insert','update','delete', 'union'
}

------------------------------------
-- A Better Naive Rule ...
------------------------------------

haka.rule {
	hooks = { 'http-request' },
	eval = function (self, pkt)
		local uri = decode_all(pkt.request.uri)
		local ck = decode_all(pkt.request.headers['Cookie'])

		local where = {args = {score = 0}, cookies = {score = 0}}

		where['args'].value = http.uri.split(uri).args
		where['cookies'].value = http.cookies.split(ck)

		for _, key in ipairs(keywords) do
			for k, v in pairs(where) do
				for param, value in pairs(where[k].value) do
					if value:find(key) then
						where[k].score = where[k].score + 4
						if where[k].score >= 8 then
							haka.log.error("filter", "SQLi attack detected !!!")
							pkt:drop()
						end
					end
				end
			end
		end
	end
}

