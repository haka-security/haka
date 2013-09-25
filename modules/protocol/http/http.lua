
local module = {}

local str = string.char

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
			http._invalid = string.format("invalid header '%s'", safe_string(line))
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
		http._invalid = string.format("invalid request '%s'", safe_string(line))
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
		http._invalid = string.format("invalid response '%s'", safe_string(line))
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
			headers[name] = nil

			stream:insert(name)
			stream:insert(": ")
			stream:insert(value)
			stream:insert("\r\n")
		end
	end

	for name, value in pairs(headers) do
		if value then
			stream:insert(name)
			stream:insert(": ")
			stream:insert(value)
			stream:insert("\r\n")
		end
	end
end

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
			http._state = 0

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
			haka.log.error("http", "%s", context._invalid)
			http._tcp_stream:drop()
			return nil
		end
	end
end


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

return module
