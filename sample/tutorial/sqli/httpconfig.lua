
------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
require('protocol/tcp')
http = require('protocol/http')

------------------------------------
-- Setting next dissector
------------------------------------

haka.rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = 'http'
		end
	end
}

------------------------------------
-- Transformation Methods
------------------------------------

-- remove sequences between /* */ comments
function uncomments(uri)
	if uri then return string.gsub(uri, '/%*.-%*/', '') end
end

-- remove null byte
function nonulls(uri)
	if uri then return string.gsub(uri, "%z", '') end
end

-- compress white space
function nospaces(uri)
	if uri then return string.gsub(uri, "%s+", " ") end
end

-- percent decode
function decode(uri)
	if uri then
		uri = string.gsub (uri, '+', ' ')
		uri = string.gsub (uri, '%%(%x%x)',
		function(h) return string.char(tonumber(h,16)) end)
		return uri
	end
end

-- lower case
lower = function(uri)
	if uri then return uri:lower() end
end

-- apply all transformation methods
decode_all = function(uri)
	if uri then
		return lower(uncomments(nospaces(nonulls(decode(uri)))))
	end
end


