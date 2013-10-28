--The require lines tolds Haka which dissector
--it has to register. Those dissectors gives hook
--to interfere with it.

require('protocol/ipv4')
require('protocol/tcp')
http = require('protocol/http')

---------------------------------------
-- Firefox Version and website
-- Be sure to update to last version
---------------------------------------
local last_firefox_version = tonumber(24.0)
local firefox_web_site = "http://www.mozilla.org"

---------------------------------------
-- Domains for updating Mozilla firefox
---------------------------------------
local update_domains = {
	'mozilla.org',
	'mozilla.net',
	--you can extend this list to other domains
}


---------------------------------------
-- Group rule to check firefox version
---------------------------------------
--This rule load http dissector for TCP port 80
haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		end
	end
}

--We create a rule group
-- This rule will allow unlimited acces to update website
-- And redirect every request to a safe site if User-Agent version is outadet

safe_update = haka.rule_group {
	name = 'safe_update',
	--initialization
	init = function (self, http)
		haka.log.info("Filter", "Domain requested: %s", http.request.headers['Host'])
	end,

	-- continue is called after evaluation of each security rule
	-- the ret parameter decide wether to read next rule or skip
	-- the evaluation of the other rules in the group
	continue = function (self, http, ret)
		return not ret
	end
}

--This rule allow access to update sites
--whatever the User-Agent version is
--It's a kind of white listing
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

safe_update:rule {
	hooks = { 'http-response' },
	eval = function (self, http)
		http.request:dump()
		local UA = http.request.headers["User-Agent"]
		haka.log.info("Filter", "UA detected: %s", UA)
		local version = tonumber(string.sub(UA, string.find(UA, "Firefox/")+8))
		haka.log.info("Filter", " Firefox version:%d",version)
		if version < last_firefox_version then
			haka.log.info("Filter", "Firefox is outdated, please upgrade")
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

