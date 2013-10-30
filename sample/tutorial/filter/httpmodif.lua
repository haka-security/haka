------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
http = require('protocol/http')

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
	-- You can extend this list to other domains
}

-------------------------------------
-- Security rule loading http dissector
-------------------------------------
haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		end
	end
}

-------------------------------------
-- Security group rule
-------------------------------------
safe_update = haka.rule_group {
	name = 'safe_update',
	-- Initialization
	init = function (self, http)
		haka.log.info("Filter", "Domain requested: %s", http.request.headers['Host'])
	end,

	-- Continue is called after evaluation of each security rule
	-- the ret parameter decide wether to read next rule or skip
	-- the evaluation of the other rules in the group
	continue = function (self, http, ret)
		return not ret
	end
}

-------------------------------------
-- Security rules
-------------------------------------
-- This rule allow access to update sites
-- whatever the User-Agent version is
-- It's a kind of white listing
safe_update:rule {
	hooks = { 'http-response' },
	eval = function (self, http)
		for _, dom in ipairs(update_domains) do
			if string.find(http.request.headers['Host'], dom) then
				haka.log.info("Filter", "Update domain: go for it", dom)
				return true
			end
		end
	end

}

-- This rule will deny access to any website if User-Agent is
-- outdated.
safe_update:rule {
	hooks = { 'http-response' },
	eval = function (self, http)
		-- Dump the request helps to debug
		http.request:dump()
		local UA = http.request.headers["User-Agent"]
		haka.log("Filter", "UA detected: %s", UA)
		local version = tonumber(string.sub(UA, string.find(UA, "Firefox/")+8))
		haka.log("Filter", "Firefox version: %d",version)
		if version < last_firefox_version then
			haka.log.info("Filter", "Firefox is outdated, please upgrade")
			-- We modify some fields of the response on the fly
			-- We redirect the browser to a safe place
			-- where updates will be made
			http.response.status = "307"
			http.response.reason = "Moved Temporarily"
			http.response.headers["Content-Length"] = "0"
			http.response.headers["Location"] = firefox_web_site
			http.response.headers["Server"] = "A patchy server"
			http.response.headers["Connection"] = "Close"
			http.response.headers["Proxy-Connection"] = "Close"
			http.response:dump()
		end
	end
}

