
------------------------------------
-- Transformation Methods
------------------------------------

-- Remove sequences between /* */ comments
function uncomments(uri)
	if uri then return string.gsub(uri, '/%*.-%*/', '') end
end

-- Remove null byte
function nonulls(uri)
	if uri then return string.gsub(uri, "%z", '') end
end

-- Compress white space
function nospaces(uri)
	if uri then return string.gsub(uri, "%s+", " ") end
end

-- Percent decode
function decode(uri)
	if uri then
		uri = string.gsub (uri, '+', ' ')
		uri = string.gsub (uri, '%%(%x%x)',
		function(h) return string.char(tonumber(h,16)) end)
		return uri
	end
end

-- Lower case
lower = function(uri)
	if uri then return uri:lower() end
end

-- Apply all transformation methods
decode_all = function(uri)
	if uri then
		return lower(uncomments(nospaces(nonulls(decode(uri)))))
	end
end
