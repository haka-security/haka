--------------------------
-- Loading dissectors
--------------------------

require('protocol/ipv4')
require('protocol/tcp')
require('protocol/http')

local tbl = require('tblutils')

-- Each entry of stats table will store
-- info about http request/response (method,
-- host, ressource, status, etc.)
stats = tbl.new()

--------------------------
-- Setting next dissector
--------------------------

haka.rule {
    hooks = { 'tcp-connection-new' },
    eval = function(self, pkt)
        local tcp = pkt.tcp
		if tcp.dstport == 80 then
			pkt.next_dissector = "http"
		end
    end
}

--------------------------
-- Recording http info
--------------------------

haka.rule {
	hooks = { 'http-response' },
	eval = function (self, http)
		local conn = http.connection
		local response = http.response
		local request = http.request
		local split_uri = request:split_uri():normalize()
		local entry = {}
		entry.ip = tostring(conn.srcip)
		entry.method = request.method
		entry.ressource = split_uri.path or ''
		entry.host = split_uri.host or ''
		entry.useragent = request.headers['User-Agent'] or ''
		entry.referer = request.headers['Referer'] or ''
		entry.status = response.status
		table.insert(stats, entry)
	end
}
