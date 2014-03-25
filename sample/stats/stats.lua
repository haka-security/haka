--------------------------
-- Loading dissectors
--------------------------

require('protocol/ipv4')
require('protocol/tcp')
local http = require('protocol/http')

local tbl = require('stats_utils')

-- Each entry of stats table will store
-- info about http request/response (method,
-- host, resource, status, etc.)
local stats = tbl.new()

--------------------------
-- Setting next dissector
--------------------------

http.install_tcp_rule(80)

--------------------------
-- Recording http info
--------------------------

haka.rule{
	hook = haka.event('http', 'response'),
	eval = function (http, response)
		local conn = http.connection
		local request = http.request
		local split_uri = request.split_uri:normalize()
		local entry = {}
		entry.ip = tostring(conn.srcip)
		entry.method = request.method
		entry.resource = split_uri.path or ''
		entry.host = split_uri.host or ''
		entry.useragent = request.headers['User-Agent'] or ''
		entry.referer = request.headers['Referer'] or ''
		entry.status = response.status
		table.insert(stats, entry)
	end
}

return stats
