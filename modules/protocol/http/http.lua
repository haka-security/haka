-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/tcp-connection")

local module = {}

local utils = require("protocol/http_utils")
table.merge(module, utils)


--
-- HTTP dissector
--

local HeaderResult = class("HeaderResult", haka.grammar.ArrayResult)

function HeaderResult.method:__init()
	rawset(self, '_cache', {})
end

function HeaderResult.method:__index(key)
	local key = key:lower()

	if self._cache[key] then
		return self._cache[key].value
	end

	for i, header in ipairs(self) do
		-- TODO avoid getting last empty header
		if header.name and header.name:lower() == key then
			self._cache[key] = header
			return header.value
		end
	end
end

function HeaderResult:__pairs()
	local i = 0
	local function headernext(headerresult, index)
		i = i + 1
		if rawget(headerresult, i) then
			return rawget(headerresult, i).name, rawget(headerresult, i).value
		else
			return nil
		end
	end
	return headernext, self, nil
end

function HeaderResult.method:__newindex(key, value)
	local lowerkey = key:lower()

	-- Try to update existing header
	for i, header in ipairs(self) do
		if header.name and header.name:lower() == lowerkey then
			if value then
				header.value = value
			else
				self:remove(i)
			end

			return
		end
	end

	-- Finally insert new header
	if value then
		self:append({ name = key, value = value })
	end
end

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

function http_dissector.method:trigger_event(res, iter, mark)
	local state = self.states.state.name
	res.http = nil

	self:trigger(state, res)

	res.http = self
end

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
http_dissector.grammar.optional_WS = haka.grammar.token('[[:blank:]]*')
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

http_dissector.grammar.headers = haka.grammar.array(http_dissector.grammar.header):options{
	untilcond = function (elem, ctx)
		local la = ctx:lookahead()
		return la == 0xa or la == 0xd
	end,
	result = HeaderResult,
	create = function (ctx, entity, init)
		local vbuf = haka.vbuffer(init.name..': '..init.value..'\r\n')
		entity:create(vbuf:pos('begin'), ctx, init)
		return vbuf
	end
}

-- http chunk
http_dissector.grammar.chunk = haka.grammar.record{
	haka.grammar.retain(),
	haka.grammar.field('chunk_size', haka.grammar.token('[0-9a-fA-F]+')
		:convert(haka.grammar.converter.tonumber("%x", 16))),
	haka.grammar.execute(function (self, ctx) ctx.chunk_size = self.chunk_size end),
	haka.grammar.release,
	http_dissector.grammar.optional_WS,
	http_dissector.grammar.CRLF,
	haka.grammar.bytes():options{
		count = function (self, ctx) return ctx.chunk_size end,
		chunked = function (self, sub, last, ctx)
			ctx.user:push_data(self, sub, last, http_dissector.events[ctx.user.states.state.name .. '_data'])
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
	http_dissector.grammar.CRLF,
	haka.grammar.release
}

http_dissector.grammar.body = haka.grammar.branch(
	{
		content = haka.grammar.bytes():options{
			count = function (self, ctx) return ctx.content_length or 0 end,
			chunked = function (self, sub, last, ctx)
				ctx.user:push_data(self, sub, last, http_dissector.events[ctx.user.states.state.name .. '_data'])
			end
		},
		chunked = http_dissector.grammar.chunks,
		default = true
	},
	function (self, ctx) return ctx.mode end
)

http_dissector.grammar.message = haka.grammar.record{
	haka.grammar.field('headers', http_dissector.grammar.headers),
	http_dissector.grammar.CRLF,
	haka.grammar.execute(function (self, ctx)
		ctx.user:trigger_event(ctx.top, ctx.iter, ctx.retain_mark)
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


--
-- HTTP parse results
--

local HttpRequestResult = class("HttpRequestResult", haka.grammar.Result)

HttpRequestResult.property.split_uri = {
	get = function (self)
		local split_uri = utils.uri.split(self.uri)
		self.split_uri = split_uri
		return split_uri
	end
}

HttpRequestResult.property.split_cookies = {
	get = function (self)
		local split_cookies = utils.cookies.split(self.headers['Cookie'])
		self.split_cookies = split_cookies
		return split_cookies
	end
}

local HttpResponseResult = class("HttpResponseResult", haka.grammar.Result)

HttpResponseResult.property.split_cookies = {
	get = function (self)
		local split_cookies = utils.cookies.split(self.headers['Set-Cookie'])
		self.split_cookies = split_cookies
		return split_cookies
	end
}


--
--  HTTP States
--

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

		self.request = HttpRequestResult:new()
		self.response = nil

		local err
		self.request, err = request:parse(iter, self.request, self)
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

		self.response = HttpResponseResult:new()

		local err
		self.response, err = response:parse(iter, self.response, self)
		if err then
			haka.alert{
				description = string.format("invalid http %s", err.rule),
				severity = 'low'
			}
			self:drop()
			return haka.abort()
		end

		if self.request.method:lower() == 'connect' then
			return context.states.connect
		else
			return context.states.request
		end
	end,
}

http_dissector.states.connect = http_dissector.states:state{
	update = function (context, direction, iter)
		iter:advance('all')
	end
}

http_dissector.states.initial = http_dissector.states.request

return module
