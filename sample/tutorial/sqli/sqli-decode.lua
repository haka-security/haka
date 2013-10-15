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


------------------------------------
-- Malicious Patterns
------------------------------------

local keywords = {
	'select','insert','update','delete', 'union'
}

-- Still naive rule
haka.rule {
	hooks = { 'http-request' },
	eval = function (self, http)
		local score = 0
		local uri = http.request.uri
		uri = uncomments(nospaces(nonulls(decode(uri)))):lower()
		for	_, key in ipairs(keywords) do
			if uri:find(key) then
				score = score + 4
				if score >= 8 then
					haka.log.error("filter", "SQLi attack detected !!!")
					http:drop()
				end
			end
		end
	end
}

