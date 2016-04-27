------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
require('protocol/http')

local last_firefox_version = 24.0
local firefox_web_site = 'http://www.mozilla.org'

-- Domain whitelist.
-- All traffic to these domains will be unmodified
local update_domains = {
	'mozilla.org',
	'mozilla.net',
	-- You can extend this list with other domains
}

-------------------------------------
-- Rule group definition
-------------------------------------
safe_update = haka.rule_group{
	on = haka.dissectors.http.events.response,

	-- Initialization
	init = function (http, response)
		local host = http.request.headers['Host']
		if host then
			haka.log("Domain requested: %s", host)
		end
	end,

	-- Continue is called after evaluation of each security rule.
	-- The ret parameter decide whether to read next rule or skip 
	-- the evaluation of the other rules in the group
	continue = function (ret)
		return not ret
	end
}

-- Traffic to all websites in the whitelist is unconditionally allowed
safe_update:rule{
	eval = function (http, response)
		local host = http.request.headers['Host'] or ''
		for _, dom in ipairs(update_domains) do
			if string.find(host, dom) then
				haka.log("Update domain: go for it")
				return true
			end
		end
	end
}

-- If the User-Agent contains firefox and the version is outdated,
-- then redirect the traffic to firefox_web_site
safe_update:rule{
	eval = function (http, response)
		-- Uncomment the following line to see the the content of the request
		-- debug.pprint(request, nil, nil, { debug.hide_underscore, debug.hide_function })

		local UA = http.request.headers["User-Agent"] or "No User-Agent header"
		haka.log("UA detected: %s", UA)
		local FF_UA = (string.find(UA, "Firefox/"))

		if FF_UA then -- Firefox was detected
			local version = tonumber(string.sub(UA, FF_UA+8))
			if not version or version < last_firefox_version then
				haka.alert{
					description= "Firefox is outdated, please upgrade",
					severity= 'medium'
				}
				-- redirect browser to a safe place where updates will be made
				response.status = "307"
				response.reason = "Moved Temporarily"
				response.headers["Content-Length"] = "0"
				response.headers["Location"] = firefox_web_site
				response.headers["Server"] = "A patchy server"
				response.headers["Connection"] = "Close"
				response.headers["Proxy-Connection"] = "Close"
				debug.pprint(response, nil, nil, { debug.hide_underscore, debug.hide_function })
			end
		else
			haka.log("Unknown or missing User-Agent")
		end
	end
}
