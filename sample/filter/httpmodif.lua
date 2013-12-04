------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
local http = require('protocol/http')

------------------------------------
-- Firefox Version and website
-- Be sure to update to last version
------------------------------------
local last_firefox_version = 24.0
local firefox_web_site = 'http://www.mozilla.org'

-------------------------------------
-- Domains for updating browsers,
-- e.g. Mozilla firefox
-------------------------------------
local update_domains = {
	'mozilla.org',
	'mozilla.net',
	-- You can extend this list with other domains
}

-------------------------------------
-- Setting http dissector
-------------------------------------
http.install_tcp_rule(80)

-------------------------------------
-- Security group rule
-------------------------------------
safe_update = haka.rule_group{
	hook = haka.event('http', 'response'),

	-- Initialization
	init = function (http, response)
		local host = http.request.headers['Host']
		if host then
			haka.log("Filter", "Domain requested: %s", host)
		end
	end,

	-- Continue is called after evaluation of each security rule the ret
	-- parameter decide whether to read next rule or skip the evaluation of
	-- the other rules in the group
	continue = function (ret)
		return not ret
	end
}

-------------------------------------
-- Security rules
-------------------------------------

-- This rule allow access to update sites whatever the User-Agent version is
-- It's a kind of white listing
safe_update:rule(
	function (http, response)
		local host = http.request.headers['Host'] or ''
		for _, dom in ipairs(update_domains) do
			if string.find(host, dom) then
				haka.log("Filter", "Update domain: go for it")
				return true
			end
		end
	end
)

-- This rule will deny access to any website if User-Agent is outdated
safe_update:rule(
	function (http, response)
		-- Uncomment to dump the request helps to debug
		-- http.request:dump()

		local UA = http.request.headers["User-Agent"] or "No User-Agent header"
		haka.log("Filter", "UA detected: %s", UA)
		local FF_UA = (string.find(UA, "Firefox/"))

		if FF_UA then -- If FF_UA is nil, the browser is not Firefox
			local version = tonumber(string.sub(UA, FF_UA+8))
			if not version or version < last_firefox_version then
				haka.alert{
					description= "Firefox is outdated, please upgrade",
					severity= 'medium'
				}
				-- We modify some fields of the response on the fly We redirect
				-- the browser to a safe place where updates will be made
				response.status = "307"
				response.reason = "Moved Temporarily"
				response.headers["Content-Length"] = "0"
				response.headers["Location"] = firefox_web_site
				response.headers["Server"] = "A patchy server"
				response.headers["Connection"] = "Close"
				response.headers["Proxy-Connection"] = "Close"
				response:dump()
			end
		else
			haka.log("Filter", "Unknown or missing User-Agent")
		end
	end
)
