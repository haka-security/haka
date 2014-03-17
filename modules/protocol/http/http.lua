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
	local headers_copy = table.dict(headers_order)
	for name, value in sorted_pairs(headers) do
		if value and not table.contains(headers_copy, name) then
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

	self:trigger(state, converted)
	self:send(iter, mark)
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

		return context.states.request
	end,
}

http_dissector.states.connect = http_dissector.states:state{
	update = function (context, iter)
		iter:advance('all')
	end
}

http_dissector.states.initial = http_dissector.states.request

return module
