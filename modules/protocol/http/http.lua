-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/tcp-connection")

local module = {}

local str = string.char


--
-- HTTP utilities
--

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


--
-- HTTP dissector
--

local function getchar(context)
	while true do
		if not context._stream:check_available(2) and not context._stream:check_available(1) then
			coroutine.yield()
		else
			return context._stream:sub(1):asnumber()
		end
	end
end

local function read_line(context)
	local line = ""
	local char, c
	local read = 0
	while true do
		c = getchar(context)
		read = read+1
		char = str(c)

		if c == 0xd then
			c = getchar(context)
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


local http_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'http'
}

http_dissector:register_event('request')
http_dissector:register_event('response')

http_dissector.property.connection = {
	get = function (self)
		self.connection = self.flow.connection
		return self.connection
	end
}

function http_dissector.method:__init(flow)
	super(http_dissector).__init(self)
	self.flow = flow
	if flow then
		self.connection = flow.connection
	end
	self._state = 0
end

function http_dissector.method:continue()
	return self.flow ~= nil
end

function http_dissector.method:drop()
	self.flow:drop()
	self.flow = nil
end

function http_dissector.method:reset()
	self.flow:reset()
	self.flow = nil
end

local function build_headers(result, headers, headers_order)
	for _, name in pairs(headers_order) do
		local value = headers[name]
		if value then
			table.insert(result, name)
			table.insert(result, ": ")
			table.insert(result, value)
			table.insert(result, "\r\n")
		end
	end
	local headers_copy = dict(headers_order)
	for name, value in sorted_pairs(headers) do
		if value and not contains(headers_copy, name) then
			table.insert(result, name)
			table.insert(result, ": ")
			table.insert(result, value)
			table.insert(result, "\r\n")
		end
	end
end

local function parse_header(http)
	local total_len = 0

	http.headers = {}
	http._headers_order = {}
	line, len = read_line(http)
	total_len = total_len + len
	while #line > 0 do
		local name, value = line:match("([^%s]+):%s*(.+)")
		if not name then
			http._invalid = string.format("invalid http header '%s'", safe_string(line))
			return 0
		end

		http.headers[name] = value
		table.insert(http._headers_order, name)
		line, len = read_line(http)
		total_len = total_len + len
	end

	return total_len
end

local function parse_request(http)
	local len, total_len

	local line, len = read_line(http)
	total_len = len
	http.method, http.uri, http.version = line:match("([^%s]+) ([^%s]+) (.+)")
	if not http.method then
		http._invalid = string.format("invalid http request '%s'", safe_string(line))
		return
	end

	total_len = total_len + parse_header(http)

	--http.data = stream
	http._length = total_len

	http.dump = dump

	return true
end

local function parse_response(http)
	local len, total_len

	local line, len = read_line(http)
	total_len = len

	http.version, http.status, http.reason = line:match("([^%s]+) ([^%s]+) (.+)")
	if not http.version then
		http._invalid = string.format("invalid http response '%s'", safe_string(line))
		return
	end

	total_len = total_len + parse_header(http)

	--http.data = stream
	http._length = total_len

	http.dump = dump

	return true
end

function http_dissector.method:parse(iter, context, f, signal, next_state)
	if not context._co then
		if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
			context._mark = iter:copy()
			context._mark:mark()
		end
		context._co = coroutine.create(function ()
			local success, msg = pcall(f, context)
			if not success then context._error = msg end
		end)
	end

	context._stream = iter
	coroutine.resume(context._co)
	context._stream = nil

	if coroutine.status(context._co) == "dead" then
		context._co = nil

		if not context._invalid and not context._error then
			self._state = next_state

			if not haka.pcall(haka.context.signal, haka.context, self, signal, context) then
				self:drop()
				return false
			end

			return true
		else
			if context._error then
				error(context._error)
			else
				haka.alert{
					description = context._invalid,
					severity = 'low'
				}
				self:drop()
			end
			return false
		end
	end
end

function http_dissector.method:receive(flow, stream, direction)
	assert(flow == self.flow)
	
	local iter = stream:pos(0)

	if direction == 'up' then
		if self._state == 0 or self._state == 1 then
			if self._state == 0 then
				self.request = {}
				self.response = nil
				self._state = 1
				
				self.request.split_uri = function (self)
					if self._splitted_uri then
						return self._splitted_uri
					else
						self._splitted_uri = uri_split(self.uri)
						return self._splitted_uri
					end
				end

				self.request.split_cookies = function (self)
					if self._cookies then
						return self._cookies
					else
						self._cookies = cookies_split(self.headers['Cookie'])
						return self._cookies
					end
				end
			end

			if self:parse(iter, self.request, parse_request, http_dissector.events.request, 2) then
				return self:send(iter, direction)
			end
		end
	else
		if self._state == 3 or self._state == 4 then
			if self._state == 3 then
				self.response = {}
				self._state = 4
			end

			if self:parse(iter, self.response, parse_response, http_dissector.events.response, 5) then
				return self:send(iter, direction)
			end
		end
	end
end

function http_dissector.method:send(iter, direction)
	if not self:continue() then
		return
	end

	if self._state == 2 and direction == 'up' then
		self._state = 3

		if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
			local request = {}
			table.insert(request, self.request.method)
			table.insert(request, " ")
			table.insert(request, self.request.uri)
			table.insert(request, " ")
			table.insert(request, self.request.version)
			table.insert(request, "\r\n")
			build_headers(request, self.request.headers, self.request._headers_order)
			table.insert(request, "\r\n")

			self.request._mark:unmark()
			self.request._mark:sub(self.request._length):replace(haka.vbuffer(table.concat(request)))
			self.request._mark = nil
		end

	elseif self._state == 5 and direction == 'down' then
		if self.request.method == 'CONNECT' then
			self._state = 6 -- We should not expect a request nor response anymore
		else
			self._state = 0
		end

		if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
			local response = {}
			table.insert(response, self.response.version)
			table.insert(response, " ")
			table.insert(response, self.response.status)
			table.insert(response, " ")
			table.insert(response, self.response.reason)
			table.insert(response, "\r\n")
			build_headers(response, self.response.headers, self.response._headers_order)
			table.insert(response, "\r\n")

			self.response._mark:unmark()
			self.response._mark:sub(self.response._length):replace(haka.vbuffer(table.concat(response)))
			self.response._mark = nil
		end
	end
end

function module.install_tcp_rule(port)
	haka.rule{
		hook = haka.event('tcp-connection', 'new_connection'),
		eval = function (flow, pkt)
			if pkt.dstport == port then
				haka.log.debug('http', "selecting http dissector on flow")
				haka.context:install_dissector(http_dissector:new(flow))
			end
		end
	}
end

http_dissector.connections = haka.events.StaticEventConnections:new()
http_dissector.connections:register(haka.event('tcp-connection', 'receive_data'), haka.events.method(haka.events.self, http_dissector.method.receive))

return module
