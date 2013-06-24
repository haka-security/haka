------------------------------------
-- HTTP Attacks
------------------------------------

-- detect malicious web scanners
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		--user-agent patterns of known web scanners
		local http_useragent = { 
			nikto	= '.+%(Nikto%/.+%)%s%(Evasions:.+%)%s%(Test:.+%)',
			nessus	= '^Nessus.*',
			w3af	= '*.%s;w3af.sf.net%)',
			sqlmap	= '^sqlmap%/.*%s%(http:%/%/sqlmap.*%)',
			fimap	= '^fimap%.googlecode%.%com.*',
			grabber	= '^Grabber.*'
		}

		if (http.request.headers['User-Agent']) then
			local user_agent = http.request.headers['User-Agent']
			for scanner, pattern in pairs(http_useragent) do
				if user_agent:match(pattern) then
					haka.log.error("filter", "'%s' scan detected !!!", scanner)
					http:drop()
				end
			end
		end
	end
}

