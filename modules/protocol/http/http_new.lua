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

local http_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'http'
}

http_dissector:register_event('request')
http_dissector:register_event('response')

--
-- HTTP State Machine
--

http_dissector.states = http.state_machine.new("http")

http_dissector.states.request = http_dissector.states:state{
	input = function (context, http)
		context.input = 'request'
		return context.states.response
	end
}

http_dissector.states.request = http_dissector.states:state{
	input = function (context, http)
		context.input = 'response'
		return context.states.request
	end
}

http_dissector.states.initial = http_dissector.states.request

--
-- HTTP Grammar
--

-- http separator tokens
local WS = haka.grammar.token('[[:blank:]]+')
local CRLF = haka.grammar.token('[%r]?%n')

-- http request version
local version = haka.grammar.record{
	haka.grammar.token('HTTP/'),
	haka.grammar.field('_num', haka.grammar.token('[0-9]+%.[0-9]+'))
}:extra{
	num = function (self)
		return toint(self._num)
	end
}

-- http response status code
local status = haka.grammar.record{
	haka.grammar.token('_num', haka.grammar.token('[0-9]{3}'))
}:extra{
	num = function (self)
		return tonumber(self._num)
	end
}

-- http request line
grammar.request_line = haka.grammar.record{
	haka.grammar.field('method', haka.grammar.token('[ˆ()<>@,;:\\"/%[%]?={}[:blank:]]+')),
	WS,
	haka.grammar.field('uri', haka.grammar.token('[[:alnum][:punct]]+')),
	WS,
	haka.grammar.field('version', version),
	CRLF
}

-- http reply line
grammar.reply_line = haka.grammar.record{
	haka.grammar.field('version', version),
	WS,
	grammar.grammar.field('status', status),
	WS,
	haka.grammar.field('reason', haka.grammar.token('[^%r%n]+')),
	CRLF
}

-- http header
local header = haka.grammar.record{
	haka.grammar.field('name', haka.grammar.token('[^:[:blank:]]+')),
	haka.grammar.token(':'),
	WS,
	haka.grammar.field('value', haka.grammar.token('[^%r%n]+')),
	CRLF
}:extra{
	function (self, context)
		local hdr_name = string.lower(self.name)
		if hdr_name == 'content_length' then
			context.content_length = tonumber(self.value)
			context.mode = 'content'
		elseif hdr_name == 'transfer-encoding' and
		       self.value == 'chunked' then
			context.mode = 'chunked'
		end
	end
}

-- http headers list
local headers = haka.grammar.record{
	haka.grammar.array(header),
	CRLF
}

-- http chunk
local chunk = haka.grammar.record{
	haka.grammar.field('_chunk_size', haka.grammar.token('[0-9a-fA-F]+')),
	CRLF,
	haka.grammar.bytes():options{
		count = function (self, context) return self.count end
	}
}:extra{
	count = function (self) return tonumber(self._chunk_size, 16) end
}

-- http chunks list
local chunks = haka.grammar.record{
	haka.grammar.array(chunk):options{
		untilcond = function (elem) return elem.count == 0 end
	},
	headers
}

local body = haka.grammar.record{
	haka.grammar.branch(
		{
			content = haka.grammar.bytes():options{
				'chunked',
				count = function (self, context) return context.content_length end
			},
			chunked = chunks
		}, 
		function (context) return context.mode end
	)
}

local message = haka.grammar.record{
	haka.grammar.field('headers', headers),
	haka.grammar.field('body', body)
}

-- http request
local request = haka.grammar.record{
	haka.grammar.field('request', request_line),
	haka.grammar.field('msg', message),
}

-- http response
local reply = haka.grammar.record{
	haka.grammar.field('response', response_line)
	haka.grammar.field('msg', message),
}


http_dissector.grammar = haka.grammar.branch(
	{
		request = request,
		response = response
	},
	function (context) return context.input end	
):compile()

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

return module
