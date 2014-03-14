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

local _unreserved = dict({string.byte("-"),
				string.byte("."),
				string.byte("_"),
				string.byte("~")})

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
		string.gsub(splitted_uri.query, '([^=&]+)(=?([^&?]*))&?',
		    function(p, q, r) args[p] = r or true return '' end)
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
		string.gsub(cookie_line, '([^=;]+)=([^;]*);?',
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

local http_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'http'
}

http_dissector:register_event('request')
http_dissector:register_event('response')
http_dissector:register_event('request_data', nil, haka.dissector.FlowDissector.stream_wrapper)
http_dissector:register_event('response_data', nil, haka.dissector.FlowDissector.stream_wrapper)

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
	self.states = http_dissector.states:instanciate()
	self.states.http = self
end

function http_dissector.method:continue()
	if not self.flow then
		haka.abort()
	end
end

function http_dissector.method:drop()
	self.flow:drop()
	self.flow = nil
end

function http_dissector.method:reset()
	self.flow:reset()
	self.flow = nil
end

function http_dissector.method:push_data(current, data, last, signal)
	if not current.data then
		current.data = haka.vbuffer_sub_stream()
	end

	current.data:push(data)
	if last then current.data:finish() end

	if not haka.pcall(haka.context.signal, haka.context, self,
			signal, current.data) then
		self:drop()
		return true
	end

	current.data:pop()
end

local function convert_headers(hdrs)
	local headers = {}
	local headers_order = {}
	for _, header in ipairs(hdrs) do
		if header.name then
			headers[header.name] = header.value
			table.insert(headers_order, header.name)
		end
	end
	return headers, headers_order
end

local convert = {}

function convert.request(request)
	request.headers, request._headers_order = convert_headers(request.headers)
	request.dump = dump
	return request
end

function convert.response(response)
	response.headers, response._headers_order = convert_headers(response.headers)
	response.dump = dump
	return response
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

function http_dissector.method:send(iter, mark)
	local len = iter.meter - mark.meter

	if self.states.state.name == 'request' then
		if haka.packet.mode() == haka.packet.PASSTHROUGH then
			return
		end

		local request = {}
		table.insert(request, self.request.method)
		table.insert(request, " ")
		table.insert(request, self.request.uri)
		table.insert(request, " HTTP/")
		table.insert(request, self.request.version)
		table.insert(request, "\r\n")
		build_headers(request, self.request.headers, self.request._headers_order)
		table.insert(request, "\r\n")

		iter:move_to(mark)
		iter:sub(len, true):replace(haka.vbuffer(table.concat(request)))
	else
		if haka.packet.mode() == haka.packet.PASSTHROUGH then
			return
		end

		local response = {}
		table.insert(response, "HTTP/")
		table.insert(response, self.response.version)
		table.insert(response, " ")
		table.insert(response, self.response.status)
		table.insert(response, " ")
		table.insert(response, self.response.reason)
		table.insert(response, "\r\n")
		build_headers(response, self.response.headers, self.response._headers_order)
		table.insert(response, "\r\n")

		iter:move_to(mark)
		iter:sub(len, true):replace(haka.vbuffer(table.concat(response)))
	end
end

function http_dissector.method:trigger_event(ctx, iter, mark)
	local state = self.states.state.name
	local converted = convert[state](ctx)
	ctx.http = nil

	self:trigger(state, converted)

	self:send(iter, mark)
	ctx.http = self
end


--
-- HTTP Grammar
--

http_dissector.grammar = haka.grammar:new("http")

local begin_grammar = haka.grammar.execute(function (self, ctx)
	ctx.mark = ctx.iter:copy()
	ctx.mark:mark(haka.packet.mode() == haka.packet.PASSTHROUGH)
end)

-- http separator tokens
http_dissector.grammar.WS = haka.grammar.token('[[:blank:]]+')
http_dissector.grammar.CRLF = haka.grammar.token('[%r]?%n')

-- http request/response version
http_dissector.grammar.version = haka.grammar.record{
	haka.grammar.token('HTTP/'),
	haka.grammar.field('version', haka.grammar.token('[0-9]+%.[0-9]+'))
}

-- http response status code
http_dissector.grammar.status = haka.grammar.record{
	haka.grammar.field('status', haka.grammar.token('[0-9]{3}'))
}

-- http request line
http_dissector.grammar.request_line = haka.grammar.record{
	haka.grammar.field('method', haka.grammar.token('[^()<>@,;:%\\"/%[%]?={}[:blank:]]+')),
	http_dissector.grammar.WS,
	haka.grammar.field('uri', haka.grammar.token('[[:alnum:][:punct:]]+')),
	http_dissector.grammar.WS,
	http_dissector.grammar.version,
	http_dissector.grammar.CRLF
}

-- http reply line
http_dissector.grammar.response_line = haka.grammar.record{
	http_dissector.grammar.version,
	http_dissector.grammar.WS,
	http_dissector.grammar.status,
	http_dissector.grammar.WS,
	haka.grammar.field('reason', haka.grammar.token('[^%r%n]+')),
	http_dissector.grammar.CRLF
}

-- headers list
http_dissector.grammar.header = haka.grammar.record{
	haka.grammar.field('name', haka.grammar.token('[^:[:blank:]]+')),
	haka.grammar.token(':'),
	http_dissector.grammar.WS,
	haka.grammar.field('value', haka.grammar.token('[^%r%n]+')),
	http_dissector.grammar.CRLF
}
:extra{
	function (self, ctx)
		local lower_name =  self.name:lower()
		if lower_name == 'content-length' then
			ctx.content_length = tonumber(self.value)
			ctx.mode = 'content'
		elseif lower_name == 'transfer-encoding' and
		       self.value:lower() == 'chunked' then
			ctx.mode = 'chunked'
		end
	end
}

http_dissector.grammar.header_or_crlf = haka.grammar.branch(
	{
		header = http_dissector.grammar.header,
		crlf = http_dissector.grammar.CRLF
	},
	function (self, ctx)
		local la = ctx:lookahead()
		if la == 0xa or la == 0xd then return 'crlf'
		else return 'header' end
	end
)

http_dissector.grammar.headers = haka.grammar.array(http_dissector.grammar.header_or_crlf):
	options{ untilcond = function (elem, ctx) return elem and not elem.name end }

-- http chunk
http_dissector.grammar.chunk = haka.grammar.record{
	haka.grammar.retain(),
	haka.grammar.field('chunk_size', haka.grammar.token('[0-9a-fA-F]+')
		:convert(haka.grammar.converter.tonumber("%x", 16))),
	haka.grammar.execute(function (self, ctx) ctx.chunk_size = self.chunk_size end),
	haka.grammar.release,
	http_dissector.grammar.CRLF,
	haka.grammar.bytes():options{
		count = function (self, ctx) return ctx.chunk_size end,
		chunked = function (self, sub, last, ctx)
			ctx.top.http:push_data(self, sub, last, http_dissector.events[ctx.top.http.states.state.name .. '_data'])
		end
	},
	haka.grammar.optional(http_dissector.grammar.CRLF,
		function (self, context) return self.chunk_size > 0 end)
}

http_dissector.grammar.chunks = haka.grammar.record{
	haka.grammar.array(http_dissector.grammar.chunk):options{
		untilcond = function (elem) return elem and elem.chunk_size == 0 end
	},
	haka.grammar.retain(),
	haka.grammar.field('headers', http_dissector.grammar.headers),
	haka.grammar.release
}

http_dissector.grammar.body = haka.grammar.branch(
	{
		content = haka.grammar.bytes():options{
			count = function (self, ctx) return ctx.content_length or 0 end,
			chunked = function (self, sub, last, ctx)
				ctx.top.http:push_data(self, sub, last, http_dissector.events[ctx.top.http.states.state.name .. '_data'])
			end
		},
		chunked = http_dissector.grammar.chunks,
		default = true
	}, 
	function (self, ctx) return ctx.mode end
)

http_dissector.grammar.message = haka.grammar.record{
	haka.grammar.field('headers', http_dissector.grammar.headers),
	haka.grammar.execute(function (self, ctx)
		ctx.top.http:trigger_event(ctx.top, ctx.iter, ctx.retain_mark)
	end),
	haka.grammar.release,
	haka.grammar.field('body', http_dissector.grammar.body)
}

-- http request
http_dissector.grammar.request = haka.grammar.record{
	haka.grammar.retain(),
	http_dissector.grammar.request_line,
	http_dissector.grammar.message
}

-- http response
http_dissector.grammar.response = haka.grammar.record{
	haka.grammar.retain(),
	http_dissector.grammar.response_line,
	http_dissector.grammar.message
}

local request = http_dissector.grammar.request:compile()
local response = http_dissector.grammar.response:compile()


http_dissector.states = haka.state_machine.new("http")

http_dissector.states:default{
	error = function (context)
		context.http:drop()
	end,
	update = function (context, direction, iter)
		return context[direction](context, iter)
	end
}

local ctx_object = class('http_ctx')

http_dissector.states.request = http_dissector.states:state{
	up = function (context, iter)
		local self = context.http

		self.request = ctx_object:new()
		self.request.http = self
		self.response = nil

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

		local err
		self.request, err = request:parse(iter, self.request)
		if err then
			haka.alert{
				description = string.format("invalid http %s", err.rule),
				severity = 'low'
			}
			self:drop()
			return haka.abort()
		end

		return context.states.response
	end,
	down = function (context)
		haka.alert{
			description = "http: unexpected data from server",
			severity = 'low'
		}
		return context.states.ERROR
	end
}

http_dissector.states.response = http_dissector.states:state{
	up = function (context, iter)
		haka.alert{
			description = "http: unexpected data from client",
			severity = 'low'
		}
		return context.states.ERROR
	end,
	down = function (context, iter)
		local self = context.http

		self.response = ctx_object:new()
		self.response.http = self

		self.response.split_cookies = function (self)
			if self._cookies then
				return self._cookies
			else
				self._cookies = cookies_split(self.headers['Set-Cookie'])
				return self._cookies
			end
		end

		local err
		self.response, err = response:parse(iter, self.response)
		if err then
			haka.alert{
				description = string.format("invalid http %s", err.rule),
				severity = 'low'
			}
			self:drop()
			return haka.abort()
		end

		return context.states.request
	end,
}

http_dissector.states.connect = http_dissector.states:state{
	update = function (context, iter)
		iter:advance('all')
	end
}

http_dissector.states.initial = http_dissector.states.request

function http_dissector.method:receive(flow, iter, direction)
	assert(flow == self.flow)

	while iter:wait() do
		self.states:update(direction, iter)
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
http_dissector.connections:register(haka.event('tcp-connection', 'receive_data'),
	haka.events.method(haka.events.self, http_dissector.method.receive),
	{streamed=true})

return module
