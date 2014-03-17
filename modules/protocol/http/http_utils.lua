-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local module = {}

module.uri = {}
module.cookies = {}


local _unreserved = table.dict({string.byte("-"),
				string.byte("."),
				string.byte("_"),
				string.byte("~")})

local str = string.char

local function uri_safe_decode(uri)
	local uri = string.gsub(uri, '%%(%x%x)',
		function(p)
			local val = tonumber(p, 16)
			if (val > 47 and val < 58) or
			   (val > 64 and val < 91) or
			   (val > 96 and val < 123) or
			   (table.contains(_unreserved, val)) then
				return str(val)
			else
				return '%' .. string.upper(p)
			end
		end)
	return uri
end

local function uri_safe_decode_split(tab)
	for k, v in pairs(tab) do
		if type(v) == 'table' then
			uri_safe_decode_split(v)
		else
			tab[k] = uri_safe_decode(v)
		end
	end
end

local _prefixes = {{'^%.%./', ''}, {'^%./', ''}, {'^/%.%./', '/'}, {'^/%.%.$', '/'}, {'^/%./', '/'}, {'^/%.$', '/'}}

local function remove_dot_segments(path)
	local output = {}
	local slash = ''
	local nb = 0
	if path:sub(1,1) == '/' then slash = '/' end
	while path ~= '' do
		local index = 0
		for _, prefix in ipairs(_prefixes) do
			path, nb = path:gsub(prefix[1], prefix[2])
			if nb > 0 then
				if index == 2 or index == 3 then
					table.remove(output, #output)
				end
				break
			end
			index = index + 1
		end
		if nb == 0 then
			if path:sub(1,1) == '/' then path = path:sub(2) end
			local left, right = path:match('([^/]*)([/]?.*)')
			table.insert(output, left)
			path = right
		end
	end
	return slash .. table.concat(output, '/')
end


-- Uri splitter object

local HttpUriSplit = class('HttpUriSplit')

function HttpUriSplit.method:__init(uri)
	assert(uri, "uri parameter required")

	local splitted_uri = {}
	local core_uri
	local query, fragment, path, authority

	-- uri = core_uri [ ?query ] [ #fragment ]
	core_uri, query, fragment =
	    string.match(uri, '([^#?]*)[%?]*([^#]*)[#]*(.*)')

	-- query (+ split params)
	if query and query ~= '' then
		self.query = query
		local args = {}
		string.gsub(self.query, '([^=&]+)(=?([^&?]*))&?',
		    function(p, q, r) args[p] = r or true return '' end)
		self.args = args
	end

	-- fragment
	if fragment and fragment ~= '' then
		self.fragment = fragment
	end

	-- scheme
	local temp = string.gsub(core_uri, '^(%a*)://',
	    function(p) if p ~= '' then self.scheme = p end return '' end)

	-- authority and path
	authority, path = string.match(temp, '([^/]*)([/]*.*)$')

	if (path and path ~= '') then
		self.path = path
	end

	-- authority = [ userinfo @ ] host [ : port ]
	if authority and authority ~= '' then
		self.authority = authority

		-- userinfo
		authority = string.gsub(authority, "^([^@]*)@",
			function(p) if p ~= '' then self.userinfo = p end return '' end)

		-- port
		authority = string.gsub(authority, ":([^:][%d]+)$",
			function(p) if p ~= '' then self.port = p end return '' end)

		-- host
		if authority ~= '' then self.host = authority end

		-- userinfo = user : password (deprecated usage)
		if self.userinfo then
			local user, pass = string.match(self.userinfo, '(.*):(.*)')
			if user and user ~= '' then
				self.user = user
				self.pass = pass
			end
		end
	end
end

function HttpUriSplit.method:__tostring()
	local uri = {}

	-- authority components
	local auth = {}

	-- host
	if self.host then
		-- userinfo
		if self.user and self.pass then
			table.insert(auth, self.user)
			table.insert(auth, ':')
			table.insert(auth, self.pass)
			table.insert(auth, '@')
		end

		table.insert(auth, self.host)

		--port
		if self.port then
			table.insert(auth, ':')
			table.insert(auth, self.port)
		end
	end

	-- scheme and authority
	if #auth > 0 then
		if self.scheme then
			table.insert(uri, self.scheme)
			table.insert(uri, '://')
			table.insert(uri, table.concat(auth))
		else
			table.insert(uri, table.concat(auth))
		end
	end

	-- path
	if self.path then
		table.insert(uri, self.path)
	end

	-- query
	if self.query then
		local query = {}
		for k, v in pairs(self.args) do
			local q = {}
			table.insert(q, k)
			table.insert(q, v)
			table.insert(query, table.concat(q, '='))
		end

		if #query > 0 then
			table.insert(uri, '?')
			table.insert(uri, table.concat(query, '&'))
		end
	end

	-- fragment
	if self.fragment then
		table.insert(uri, '#')
		table.insert(uri, self.fragment)
	end

	return table.concat(uri)
end


function HttpUriSplit.method:normalize()
	assert(self)
	-- decode percent-encoded octets of unresserved chars
	-- capitalize letters in escape sequences
	uri_safe_decode_split(self)

	-- use http as default scheme
	if not self.scheme and self.authority then
		self.scheme = 'http'
	end

	-- scheme and host are not case sensitive
	if self.scheme then self.scheme = string.lower(self.scheme) end
	if self.host then self.host = string.lower(self.host) end

	-- remove default port
	if self.port and self.port == '80' then
		self.port = nil
	end

	-- add '/' to path
	if self.scheme == 'http' and (not self.path or self.path == '') then
		self.path = '/'
	end

	-- normalize path according to rfc 3986
	if self.path then self.path = remove_dot_segments(self.path) end

	return self

end


function module.uri.split(uri)
	return HttpUriSplit:new(uri)
end

function module.uri.normalize(uri)
	local splitted_uri = HttpUriSplit:new(uri)
	splitted_uri:normalize()
	return tostring(splitted_uri)
end


-- Cookie slitter object

local HttpCookiesSplit = class('HttpCookiesSplit')

function HttpCookiesSplit.method:__init(cookie_line)
	if cookie_line then
		string.gsub(cookie_line, '([^=;]+)=([^;]*);?',
			function(p, q) self[p] = q return '' end)
	end
end

function HttpCookiesSplit.method:__tostring()
	assert(self)
	local cookie = {}
	for k, v in pairs(self) do
		local ck = {}
		table.insert(ck, k)
		table.insert(ck, v)
		table.insert(cookie, table.concat(ck, '='))
	end
	return table.concat(cookie, ';')
end

function module.cookies.split(cookie_line)
	return HttpCookiesSplit:new(cookie_line)
end

return module
