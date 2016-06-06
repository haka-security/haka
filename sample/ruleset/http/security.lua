------------------------------------
-- HTTP Attacks
------------------------------------

-- detect malicious web scanners
haka.rule {
	on = haka.dissectors.http.events.request,
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
					haka.alert{
						description = string.format("'%s' scan detected", scanner),
						severity = 'high',
						sources = haka.alert.address(http.srcip),
						targets = {
							haka.alert.address(http.dstip),
							haka.alert.service(string.format("tcp/%d", http.dstport), "http")
						},
					}
					http:drop()
					return
				end
			end
		end
	end
}

