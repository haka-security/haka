
local module = {}

local http = haka.dissector.new('http', { 'http-request', 'http-response' })

static(http).getdata = function (cls, stream)
	local data = stream.connection.data._http
	if not data then
		data = cls:new(stream)
		stream.connection.data._http = data
	end

	data._tcp_stream = stream
	return data
end

function http:__init(stream)
	self.__super.__init(self)
	self.connection = stream.connection
	self._state = 0
end

function http:valid()
	return self._tcp_stream:valid()
end

function http:drop()
	return self._tcp_stream:drop()
end

function http:reset()
	return self._tcp_stream:reset()
end

function http:forge()
	local tcp = self._tcp_stream
	if tcp then
		if self._state == 2 and tcp.direction then
			self._state = 3

			if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
				tcp.stream:seek(self.request._mark, true)
				self.request._mark = nil

				tcp.stream:erase(self.request._length)
				tcp.stream:insert(self.request.method)
				tcp.stream:insert(" ")
				tcp.stream:insert(self.request.uri)
				tcp.stream:insert(" ")
				tcp.stream:insert(self.request.version)
				tcp.stream:insert("\r\n")
				build_headers(tcp.stream, self.request.headers, self.request._headers_order)
				tcp.stream:insert("\r\n")
			end

		elseif self._state == 5 and not tcp.direction then
			self._state = 0

			if haka.packet.mode() ~= haka.packet.PASSTHROUGH then
				tcp.stream:seek(self.response._mark, true)
				self.response._mark = nil

				tcp.stream:erase(self.response._length)
				tcp.stream:insert(self.response.version)
				tcp.stream:insert(" ")
				tcp.stream:insert(self.response.status)
				tcp.stream:insert(" ")
				tcp.stream:insert(self.response.reason)
				tcp.stream:insert("\r\n")
				build_headers(tcp.stream, self.response.headers, self.response._headers_order)
				tcp.stream:insert("\r\n")
			end
		end

		self._tcp_stream = nil
	end
	return tcp
end

function http:dissect()
	if self._tcp_stream.direction then
		if self._state == 0 or self._state == 1 then
			if self._tcp_stream.stream:available() > 0 then
				if self._state == 0 then
					self.request = {}
					self.response = nil
					self._state = 1
				end

				parse(self, self.request, parse_request, "request", 2)
			end
		elseif self.request then
			self.next_dissector = self.request.next_dissector
		end
	else
		if self._state == 3 or self._state == 4 then
			if self._tcp_stream.stream:available() > 0 then
				if self._state == 3 then
					self.response = {}
					self._state = 4
				end

				parse(self, self.response, parse_response, "response", 5)
			end
		elseif self.response then
			self.next_dissector = self.response.next_dissector
		end
	end
end


local states = http.states

states.request = states:state {
	[haka.state_machine.on_input("request")] = function (http, request)
		http.request = request
		return "reponse"
	end,
	[haka.state_machine.on_timeout(100)] = function (http)
		return "ERROR"
	end
}

states.response = states:state {
	[haka.state_machine.on_input("response")] = function (http, response)
		http.response = response
		return "request"
	end,
	[haka.state_machine.on_timeout(100)] = function (http)
		return "ERROR"
	end
}

states.initial = "request"


http.grammar = haka.grammar.new()
local grammar = http.grammar

grammar.WS = haka.grammar.re('[ \t]+')
grammar.CRLF = haka.grammar.re('[\r]?\n')
grammar.TOKEN = haka.grammar.re('[^()<>@,;:\\\"\\/\\[\\]?={} \t]+')
grammar.URI = haka.grammar.re('[A-Za-z\\.]+')

grammar.version = haka.grammar.record{
	haka.grammar.re('HTTP/'),
	haka.grammar.re('[0-9]+\\.[0-9]+'):as('_num'),
}:extra{
	num = function (self)
		return toint(self._num)
	end
}

grammar.status = haka.grammar.record{
	haka.grammar.re('[0-9]{3}'):as('_num'),
}:extra{
	num = function (self)
		return tonumber(self._num)
	end
}

grammar.request_line = haka.grammar.record{
	grammar.TOKEN:as('method'),
	grammar.WS,
	grammar.URI:as('uri'),
	grammar.WS,
	grammar.version:as('version'),
	grammar.CRLF
}

grammar.reply_line = haka.grammar.record{
	grammar.version:as('version'),
	grammar.WS,
	grammar.status:as('status'),
	grammar.WS,
	haka.grammar.re('[^\r\n]+'):as('reason'),
	grammar.CRLF
}

grammar.header = haka.grammar.record{
	haka.grammar.re('[^: \t]+'):as('name'),
	haka.grammar.re(':'),
	grammar.WS,
	haka.grammar.re('[^\r\n]+'):as('value'),
	grammar.CRLF
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

grammar.headers = haka.grammar.record{
	haka.grammar.array(grammar.header),
	grammar.CRLF
}

grammar.chunk = haka.grammar.record{
	haka.grammar.re('[0-9a-fA-F]+'):as('_chunk_size'),
	grammar.CRLF,
	haka.grammar.bytes():options{
		count = function (self, context) return self.count end
	}
}:extra{
	count = function (self) return tonumber(self._chunk_size, 16) end
}

grammar.chunks = haka.grammar.record{
	haka.grammar.array(grammar.chunk):options{
		untilcond = function (elem) return elem.count == 0 end
	},
	grammar.headers
}

grammar.body = haka.grammar.record{
	haka.grammar.branch(
		{
			content = haka.grammar.bytes():options{
				'chunked',
				count = function (self, context) return context.content_length end
			},
			chunked = grammar.chunks
		}, 
		function (context) return context.mode end
	)
}

grammar.message = haka.grammar.record{
	grammar.headers:as('headers'),
	grammar.body:as('body')
}

grammar.request = haka.grammar.record{
	grammar.request_line:as('request'),
	grammar.message:as('msg')
}

grammar.reply = haka.grammar.record{
	grammar.reply_line:as('reply'),
	grammar.message:as('msg')
}

return module
