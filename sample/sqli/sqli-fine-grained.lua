
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

haka.rule{
	hook = httplib.events.request,
	eval = function (http, request)
		dump_request(request)

		local uri = request.uri
		local ck = request.headers['Cookie']

		-- Initialize the score for query's argument and cookies list
		-- Could be extended to check patterns in other http fields
		local where = {
			args = {
				-- Split query into list of (param-name, param-value) pairs
				value = httplib.uri.split(uri).args,
				score = 0
			},
			cookies = {
				-- Split comma-separated cookies into a list of (key, value)
				-- pairs
				value = httplib.cookies.split(ck),
				score = 0
			}
		}

		for k, v in pairs(where) do
			if v.value then
				-- Loop on each query param | cookie value
				for param, value in pairs(v.value) do
					local decoded = decode_all(value)
					for _, key in ipairs(keywords) do
						if decoded:find(key) then
							v.score = v.score + 4
						end
					end
				end
			end

			if v.score >= 8 then
				-- Report an alert (more info in alert)
				haka.alert{
					description = string.format("SQLi attack detected in %s with score %d", k, v.score),
					severity = 'high',
					confidence = 'medium',
					sources = haka.alert.address(http.flow.srcip),
					targets = {
						haka.alert.address(http.flow.dstip),
						haka.alert.service(string.format("tcp/%d", http.flow.dstport), "http")
					},
				}
				http:drop()
				return
			end
		end
	end
}

