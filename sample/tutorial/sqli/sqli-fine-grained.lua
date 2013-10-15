
require('httpconfig')

------------------------------------
-- Transformation Methods
------------------------------------

-- remove sequences between /* */ comments
local function uncomments(uri)
	return string.gsub(uri, '/%*.-%*/', '')
end

-- remove null byte
local function nonulls(uri)
	return string.gsub(uri, "%z", '')
end

-- compress white space
local function nospaces(uri)
	return string.gsub(uri, "%s+", " ")
end

-- percent decode
local function decode(uri)
	uri = string.gsub (uri, '+', ' ')
	uri = string.gsub (uri, '%%(%x%x)',
	function(h) return string.char(tonumber(h,16)) end)
	return uri
end

local function decode_all(uri)
	return uncomments(nospaces(nonulls(decode(uri)))):lower()
end


------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select','insert','update','delete', 'union'
}

------------------------------------
-- A Better Naive Rule ...
------------------------------------

haka.rule {
	hooks = { 'http-request' },
	eval = function (self, pkt)
		local uri = decode_all(pkt.request.uri)
		local ck = decode_all(pkt.request.headers['Cookie'])

		local where = {args = {score = 0}, cookies = {score = 0}}

		where['args'].value = http.uri.split(uri).args
		where['cookies'].value = http.cookies.split(ck)

		for	_, key in ipairs(keywords) do
			for k, _ in pairs(where) do
				for param, value in pairs(where[k].value) do
					if value:find(key) then
						where[k].score = where[k].score + 4
						if where[k].score >= 8 then
							haka.log.error("filter", "SQLi attack detected !!!")
							pkt:drop()
						end
					end
				end
			end
		end
	end
}

