-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local module = {}

local str = string.char

local function contains(table, elem)
	return table[elem] ~= nil
end

local function dict(table)
	local ret = {}
	for _, v in pairs(table) do
		ret[v] = true
	end
	return ret
end

local function getchar(stream)
	local char

	while true do
		char = stream:getchar()
		if char == -1 then
			coroutine.yield()
		else
			break
		end
	end

	return char
end

local function read_line(stream)
	local line = ""
	local char, c
	local read = 0

	while true do
		c = getchar(stream)
		read = read+1
		char = str(c)

		if c == 0xd then
			c = getchar(stream)
			read = read+1

			if c == 0xa then
				return line, read
			else
				line = line .. char
				char = str(c)
			end
		elseif c == 0xa then
			return line, read
		end

		line = line .. char
	end
end

-- The comparison is broken in Lua 5.1, so we need to reimplement the
-- string comparison
local function string_compare(a, b)
	if type(a) == "string" and type(b) == "string" then
		local i = 1
		local sa = #a
		local sb = #b

		while true do
			if i > sa then
				return false
			elseif i > sb then
				return true
			end

			if a:byte(i) < b:byte(i) then
				return true
			elseif a:byte(i) > b:byte(i) then
				return false
			end

			i = i+1
		end

		return false
	else
		return a < b
	end
end

local function sorted_pairs(t)
	local a = {}
	for n in pairs(t) do
		table.insert(a, n)
	end

	table.sort(a, string_compare)
	local i = 0
	return function ()   -- iterator function
		i = i + 1
		if a[i] == nil then return nil
		else return a[i], t[a[i]]
		end
	end
end

local function dump(t, indent)
	if not indent then indent = "" end

	for n, v in sorted_pairs(t) do
		if type(v) == "table" then
			print(indent, n)
			dump(v, indent .. "  ")
		elseif type(v) ~= "thread" and
			type(v) ~= "userdata" and
			type(v) ~= "function" then
			print(indent, n, "=", v)
		end
	end
end

local function safe_string(str)
	local len = #str
	local sstr = {}

	for i=1,len do
		local b = str:byte(i)

		if b >= 0x20 and b <= 0x7e then
			sstr[i] = string.char(b)
		else
			sstr[i] = string.format('\\x%x', b)
		end
	end

	return table.concat(sstr)
end

local function parse_header(stream, http)
	local total_len = 0

	http.headers = {}
	http._headers_order = {}
	line, len = read_line(stream)
	total_len = total_len + len
	while #line > 0 do
		local name, value = line:match("([^%s]+):%s*(.+)")
		if not name then
			http._invalid = string.format("invalid http header '%s'", safe_string(line))
			return
		end

		http.headers[name] = value
		table.insert(http._headers_order, name)
		line, len = read_line(stream)
		total_len = total_len + len
	end

	return total_len
end

local function parse_request(stream, http)
	local len, total_len

	local line, len = read_line(stream)
	total_len = len

	http.method, http.uri, http.version = line:match("([^%s]+) ([^%s]+) (.+)")
	if not http.method then
		http._invalid = string.format("invalid http request '%s'", safe_string(line))
		return
	end

	total_len = total_len + parse_header(stream, http)

	http.data = stream
	http._length = total_len

	http.dump = dump

	return true
end

local function parse_response(stream, http)
	local len, total_len

	local line, len = read_line(stream)
	total_len = len

	http.version, http.status, http.reason = line:match("([^%s]+) ([^%s]+) (.+)")
	if not http.version then
		http._invalid = string.format("invalid http response '%s'", safe_string(line))
		return
	end

	total_len = total_len + parse_header(stream, http)

	http.data = stream
	http._length = total_len

	http.dump = dump

	return true
end

local function build_headers(stream, headers, headers_order)

	for _, name in pairs(headers_order) do
		local value = headers[name]
		if value then
			stream:insert(name)
			stream:insert(": ")
			stream:insert(value)
			stream:insert("\r\n")
		end
	end
	local headers_copy = dict(headers_order)
	for name, value in pairs(headers) do
		if value and not contains(headers_copy, name) then
			stream:insert(name)
			stream:insert(": ")
			stream:insert(value)
			stream:insert("\r\n")
		end
	end
end

local _unreserved = dict({45, 46, 95, 126})

local function uri_safe_decode(uri)
	local uri = string.gsub(uri, '%%(%x%x)',
		function(p)
			local val = tonumber(p, 16)
			if (val > 47 and val < 58) or
			   (val > 64 and val < 91) or
			   (val > 96 and val < 123) or
			   (contains(_unreserved, val)) then
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

-- register methods on splitted uri object
local mt_uri = {}
mt_uri.__index = mt_uri

function mt_uri:__tostring()
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


function mt_uri:normalize()
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


local function uri_split(uri)
	if not uri then return nil end

	local splitted_uri = {}
	local core_uri
	local query, fragment, path, authority

	setmetatable(splitted_uri, mt_uri)

	-- uri = core_uri [ ?query ] [ #fragment ]
	core_uri, query, fragment =
	    string.match(uri, '([^#?]*)[%?]*([^#]*)[#]*(.*)')

	-- query (+ split params)
	if query and query ~= '' then
		splitted_uri.query = query
		local args = {}
		string.gsub(splitted_uri.query, '([^=&]+)=([^&?]*)&?',
		    function(p, q) args[p] = q return '' end)
		splitted_uri.args = args
	end

	-- fragment
	if fragment and fragment ~= '' then
		splitted_uri.fragment = fragment
	end

	-- scheme
	local temp = string.gsub(core_uri, '^(%a*)://',
	    function(p) if p ~= '' then splitted_uri.scheme = p end return '' end)

	-- authority and path
	authority, path = string.match(temp, '([^/]*)([/]*.*)$')

	if (path and path ~= '') then
		splitted_uri.path = path
	end

	-- authority = [ userinfo @ ] host [ : port ]
	if authority and authority ~= '' then
		splitted_uri.authority = authority
		-- userinfo
		authority = string.gsub(authority, "^([^@]*)@",
		    function(p) if p ~= '' then splitted_uri.userinfo = p end return '' end)
		-- port
		authority = string.gsub(authority, ":([^:][%d]+)$",
		    function(p) if p ~= '' then splitted_uri.port = p end return '' end)
		-- host
		if authority ~= '' then splitted_uri.host = authority end
		-- userinfo = user : password (deprecated usage)
		if not splitted_uri.userinfo then return splitted_uri end

		local user, pass = string.match(splitted_uri.userinfo, '(.*):(.*)')
		if user and user ~= '' then
			splitted_uri.user = user
			splitted_uri.pass = pass
		end
	end
	return splitted_uri
end

local function uri_normalize(uri)
	local splitted_uri = uri_split(uri)
	splitted_uri:normalize()
	return tostring(splitted_uri)
end


-- register methods on splitted cookie list
local mt_cookie = {}
mt_cookie.__index = mt_cookie

function mt_cookie:__tostring()
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

local function cookies_split(cookie_line)
	local cookies = {}
	if cookie_line then
		string.gsub(cookie_line, '([^=;]+)=([^;?]*);?',
		    function(p, q) cookies[p] = q return '' end)
	end
	setmetatable(cookies, mt_cookie)
	return cookies
end

module.uri = {}
module.cookies = {}
module.uri.split = uri_split
module.uri.normalize = uri_normalize
module.cookies.split = cookies_split

local function forge(http)
	local tcp = http._tcp_stream
	if tcp then
		if http._state == 2 and tcp.direction then
			http._state = 3

			if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
				tcp.stream:seek(http.request._mark, true)
				http.request._mark = nil

				tcp.stream:erase(http.request._length)
				tcp.stream:insert(http.request.method)
				tcp.stream:insert(" ")
				tcp.stream:insert(http.request.uri)
				tcp.stream:insert(" ")
				tcp.stream:insert(http.request.version)
				tcp.stream:insert("\r\n")
				build_headers(tcp.stream, http.request.headers, http.request._headers_order)
				tcp.stream:insert("\r\n")
			end

		elseif http._state == 5 and not tcp.direction then
			if http.request.method == 'CONNECT' then
				http._state = 6 -- We should not expect a request nor response anymore
			else
				http._state = 0
			end

			if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
				tcp.stream:seek(http.response._mark, true)
				http.response._mark = nil

				tcp.stream:erase(http.response._length)
				tcp.stream:insert(http.response.version)
				tcp.stream:insert(" ")
				tcp.stream:insert(http.response.status)
				tcp.stream:insert(" ")
				tcp.stream:insert(http.response.reason)
				tcp.stream:insert("\r\n")
				build_headers(tcp.stream, http.response.headers, http.response._headers_order)
				tcp.stream:insert("\r\n")
			end
		end

		http._tcp_stream = nil
	end
	return tcp

end

local function parse(http, context, f, name, next_state)
	if not context._co then
		if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
			context._mark = http._tcp_stream.stream:mark()
		end
		context._co = coroutine.create(function () f(http._tcp_stream.stream, context) end)
	end

	coroutine.resume(context._co)

	if coroutine.status(context._co) == "dead" then
		if not context._invalid then
			http._state = next_state
			if not haka.rule_hook("http-".. name, http) then
				return nil
			end

			context.next_dissector = http.next_dissector
		else
			haka.alert{
				description = context._invalid,
				severity = 'low'
			}
			http._tcp_stream:drop()
			return nil
		end
	end
end

haka.dissector {
	name = "http",
	hooks = { "http-request", "http-response" },
	dissect = function (stream)

		if not stream.connection.data._http then
			local http = {}
			http.dissector = "http"
			http.next_dissector = nil
			http.valid = function (self)
				return self._tcp_stream:valid()
			end
			http.drop = function (self)
				return self._tcp_stream:drop()
			end
			http.reset = function (self)
				return self._tcp_stream:reset()
			end
			http.forge = forge
			http._state = 0
			http.connection = stream.connection

			stream.connection.data._http = http
		end

		local http = stream.connection.data._http
		http._tcp_stream = stream

		if stream.direction then
			if http._state == 0 or http._state == 1 then
				if stream.stream:available() > 0 then
					if http._state == 0 then
						http.request = {}
						http.request.split_uri = function (self)
							if self._splitted_uri then
								return self._splitted_uri
							else
								self._splitted_uri = uri_split(self.uri)
								return self._splitted_uri
							end
						end

						http.request.split_cookies = function (self)
							if self._cookies then
								return self._cookies
							else
								self._cookies = cookies_split(self.headers['Cookie'])
								return self._cookies
							end
						end

						http.response = nil
						http._state = 1
					end
					parse(http, http.request, parse_request, "request", 2)
				end
			elseif http.request then
				http.next_dissector = http.request.next_dissector
			end
		else
			if http._state == 3 or http._state == 4 then
				if stream.stream:available() > 0 then
					if http._state == 3 then
						http.response = {}
						http._state = 4
					end

					parse(http, http.response, parse_response, "response", 5)
				end
			elseif http.response then
				http.next_dissector = http.response.next_dissector
			end
		end

		return http
	end
}

return module
