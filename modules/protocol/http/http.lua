-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local tcp_connection = require("protocol/tcp_connection")

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
http_dissector:register_event('receive_data', nil, haka.dissector.FlowDissector.stream_wrapper)

http_dissector.property.connection = {
	get = function (self)
		self.connection = self.flow.connection
		return self.connection
	end
}

function http_dissector.method:__init(flow)
	class.super(http_dissector).__init(self)
	self.flow = flow
	self.state = http_dissector.states:instanciate(self)
	self._want_data_modification = false
end

function http_dissector.method:enable_data_modification()
	self._want_data_modification = true
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

function http_dissector.method:push_data(current, data, iter, last, state, chunk)
	if not current.data then
		current.data = haka.vbuffer_sub_stream()
	end

	local currentiter = nil
	if data then
		currentiter = current.data:push(data)
	end

	if last then current.data:finish() end

	if data or last then
		if state == 'request' then
			self:trigger('receive_data', current.data, currentiter, 'up')
		else
			self:trigger('receive_data', current.data, currentiter, 'down')
		end

		self:trigger(state..'_data', current.data, currentiter)
	end

	local sub
	if data then
		sub = current.data:pop()
	end

	if self._enable_data_modification then
		if sub then
			if #sub > 0 then
				-- create a chunk
				sub:pos('begin'):insert(haka.vbuffer_from(string.format("%x\r\n", #sub)))
				sub:pos('end'):insert(haka.vbuffer_from("\r\n"))
			end
		end

		if last then
			if chunk then
				iter:insert(haka.vbuffer_from("0\r\n"))
			else
				iter:insert(haka.vbuffer_from("0\r\n\r\n"))
			end
		end
	end

	if last then
		current.data = nil
	end
end

function http_dissector.method:trigger_event(res, iter, mark)
	local state = self.state.current

	self:trigger(state, res)

	if self._want_data_modification then
		res.headers['Content-Length'] = nil
		res.headers['Transfer-Encoding'] = 'chunked'
		self._enable_data_modification = true
	else
		self._enable_data_modification = false
	end
end

function http_dissector.method:receive(stream, current, direction)
	return haka.dissector.pcall(self, function ()
		if not self._receive_callback then
			self._receive_callback = haka.event.method(self, http_dissector.method.receive_streamed)
		end
	
		self.flow:streamed(self._receive_callback, stream, current, direction)
		
		if self.flow then
			self.flow:send(direction)
		end
	end)
end

function http_dissector.method:receive_streamed(flow, iter, direction)
	assert(flow == self.flow)

	while iter:wait() do
		self.state:transition('update', direction, iter)
		self:continue()
	end
end

function module.dissect(flow)
	flow:select_next_dissector(http_dissector:new(flow))
end

function module.install_tcp_rule(port)
	haka.rule{
		hook = tcp_connection.events.new_connection,
		eval = function (flow, pkt)
			if pkt.dstport == port then
				haka.log.debug('http', "selecting http dissector on flow")
				module.dissect(flow)
			end
		end
	}
end


--
-- HTTP parse results
--

local HeaderResult = class.class("HeaderResult", haka.grammar.result.ArrayResult)

function HeaderResult.method:__init()
	rawset(self, '_cache', {})
end

function HeaderResult.method:__index(key)
	local key = key:lower()

	local cache = self._cache[key]
	if cache and cache.name == key then
		return cache.value
	end

	for i, header in ipairs(self) do
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
		local result = rawget(headerresult, i)
		if result then
			return result.name, result.value
		else
			return nil
		end
	end
	return headernext, self, nil
end

function HeaderResult.method:__newindex(key, value)
	local lowerkey = key
	if type(lowerkey) == 'string' then
		lowerkey = lowerkey:lower()
	end

	-- Try to update existing header
	for i, header in ipairs(self) do
		if header.name and header.name:lower() == lowerkey then
			if value then
				header.value = value
			else
				self:remove(i)
				self._cache[lowerkey] = nil
			end

			return
		end
	end

	-- Finally insert new header
	if value then
		self:append({ name = key, value = value })
	end
end


local HttpRequestResult = class.class("HttpRequestResult", haka.grammar.Result)

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

local HttpResponseResult = class.class("HttpResponseResult", haka.grammar.Result)

HttpResponseResult.property.split_cookies = {
	get = function (self)
		local split_cookies = utils.cookies.split(self.headers['Set-Cookie'])
		self.split_cookies = split_cookies
		return split_cookies
	end
}


--
-- HTTP Grammar
--

http_dissector.grammar = haka.grammar.new("http", function ()
	-- http separator tokens
	WS = token('[[:blank:]]+')
	optional_WS = token('[[:blank:]]*')
	CRLF = token('[%r]?%n')

	-- http request/response version
	version = record{
		token('HTTP/'),
		field('version', token('[0-9]+%.[0-9]+'))
	}

	-- http response status code
	status = record{
		field('status', token('[0-9]{3}'))
	}

	-- http request line
	request_line = record{
		field('method', token('[^()<>@,;:%\\"/%[%]?={}[:blank:]]+')),
		WS,
		field('uri', token('[[:alnum:][:punct:]]+')),
		WS,
		version,
		CRLF
	}

	-- http reply line
	response_line = record{
		version,
		WS,
		status,
		WS,
		field('reason', token('[^%r%n]+')),
		CRLF
	}

	-- headers list
	header = record{
		field('name', token('[^:[:blank:]]+')),
		token(':'),
		WS,
		field('value', token('[^%r%n]+')),
		CRLF
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

	headers = record{
		field('headers', array(header):options{
			untilcond = function (elem, ctx)
				local la = ctx:lookahead()
				return la == 0xa or la == 0xd
			end,
			result = HeaderResult,
			create = function (ctx, entity, init)
				local vbuf = haka.vbuffer_from(init.name..': '..init.value..'\r\n')
				entity:create(vbuf:pos('begin'), ctx, init)
				return vbuf
			end
		}),
		CRLF
	}

	-- http chunk
	local erase_since_retain = execute(function (self, ctx)
		if ctx.user._enable_data_modification then
			local sub = haka.vbuffer_sub(ctx.retain_mark, ctx.iter)
			ctx.iter:split()
			sub:erase()
		end
	end)

	chunk_end_crlf = record{
		CRLF,
		erase_since_retain
	}

	chunk_line = record{
		field('chunk_size', token('[0-9a-fA-F]+')
			:convert(converter.tonumber("%x", 16))),
		execute(function (self, ctx) ctx.chunk_size = self.chunk_size end),
		optional_WS,
		CRLF,
		erase_since_retain
	}

	chunk = sequence{
		chunk_line,
		bytes():options{
			count = function (self, ctx) return ctx.chunk_size end,
			chunked = function (self, sub, last, ctx)
				ctx.user:push_data(ctx:result(1), sub, ctx.iter, ctx.chunk_size == 0,
					ctx.user.state.current, true)
			end
		},
		optional(chunk_end_crlf,
			function (self, context) return self.chunk_size > 0 end)
	}

	chunks = sequence{
		array(chunk):options{
			untilcond = function (elem) return elem and elem.chunk_size == 0 end
		},
		headers
	}

	body = branch(
		{
			content = bytes():options{
				count = function (self, ctx) return ctx.content_length or 0 end,
				chunked = function (self, sub, last, ctx)
					ctx.user:push_data(ctx:result(1), sub, ctx.iter, last,
						ctx.user.state.current)
				end
			},
			chunked = chunks,
			default = 'continue'
		},
		function (self, ctx) return ctx.mode end
	)

	message = record{
		headers,
		execute(function (self, ctx)
			ctx.user:trigger_event(ctx.top, ctx.iter, ctx.retain_mark)
		end),
		field('body', body)
	}

	-- http request
	request_headers = record{
		request_line,
		headers,
		execute(function (self, ctx)
			ctx.user:trigger_event(ctx:result(1), ctx.iter, ctx.retain_mark)
		end)
	}

	request = sequence{
		request_headers,
		body
	}

	-- http response
	response_headers = record{
		response_line,
		headers,
		execute(function (self, ctx)
			ctx.user:trigger_event(ctx:result(1), ctx.iter, ctx.retain_mark)
		end)
	}

	response = sequence{
		response_headers,
		body
	}

	export(request, response)
end)


--
--  HTTP States
--

http_dissector.states = haka.state_machine("http", function ()
	default_transitions{
		error = function (self)
			self:drop()
		end,
		update = function (self, direction, iter)
			return self.state:transition(direction, iter)
		end
	}

	request = state{
		up = function (self, iter)
			self.request = HttpRequestResult:new()
			self.response = nil
			self._want_data_modification = false

			local err
			self.request, err = http_dissector.grammar.request:parse(iter, self.request, self)
			if err then
				haka.alert{
					description = string.format("invalid http %s", err.field.rule),
					severity = 'low'
				}
				return 'error'
			end

			return 'response'
		end,
		down = function (self)
			haka.alert{
				description = "http: unexpected data from server",
				severity = 'low'
			}
			return 'error'
		end
	}

	response = state{
		up = function (self, iter)
			haka.alert{
				description = "http: unexpected data from client",
				severity = 'low'
			}
			return 'error'
		end,
		down = function (self, iter)
			self.response = HttpResponseResult:new()
			self._want_data_modification = false

			local err
			self.response, err = http_dissector.grammar.response:parse(iter, self.response, self)
			if err then
				haka.alert{
					description = string.format("invalid http %s", err.field.rule),
					severity = 'low'
				}
				return 'error'
			end

			if self.request.method:lower() == 'connect' then
				return 'connect'
			else
				return 'request'
			end
		end,
	}

	connect = state{
		update = function (self, direction, iter)
			iter:advance('all')
		end
	}

	initial = request
end)

module.events = http_dissector.events

return module
