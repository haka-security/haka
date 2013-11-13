------------------------------------
-- HTTP Attacks
------------------------------------

-- detect malicious web scanners
haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		--user-agent patterns of known web scanners
		local http_useragent = {
			nikto	= '.+%(Nikto%/.+%)%s%(Evasions:.+%)%s%(Test:.+%)',
			nessus	= '^Nessus.*',
			w3af	= '*.%s;w3af.sf.net%)',
			sqlmap	= '^sqlmap%/.*%s%(http:%/%/sqlmap.*%)',
			fimap	= '^fimap%.googlecode%.%com.*',
			grabber	= '^Grabber.*'
		}

		if request.headers['User-Agent'] then
			local user_agent = request.headers['User-Agent']
			for scanner, pattern in pairs(http_useragent) do
				if user_agent:match(pattern) then
					local conn = http.connection
					haka.alert{
						description = string.format("'%s' scan detected", scanner),
						severity = 'low',
						sources = haka.alert.address(conn.srcip),
						targets = {
							haka.alert.address(conn.dstip),
							haka.alert.service(string.format("tcp/%d", conn.dstport), "http")
						},
					}
					http:drop()
				end
			end
		end
	end
}

