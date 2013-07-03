module("http", package.seeall)

local function str(char)
	return string.char(char)
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

function sorted_pairs(t)
	local a = {}
	for n in pairs(t) do
		table.insert(a, n)
	end

	table.sort(a)
	local i = 0
	local iter = function ()   -- iterator function
		i = i + 1
		if a[i] == nil then return nil
		else return a[i], t[a[i]]
		end
	end
	return iter
end

local function dump(t, indent)
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

local function parse_header(stream, http)
	local total_len = 0

	http.headers = {}
	http._headers_order = {}
	line, len = read_line(stream)
	total_len = total_len + len
	while #line > 0 do
		local name, value = line:match("([^%s]+):%s*(.+)")
		if not name then
			http._invalid = string.format("invalid header '%s'", line)
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
		http._invalid = string.format("invalid request '%s'", line)
		return
	end

	total_len = total_len + parse_header(stream, http)

	http.data = stream
	http._length = total_len

	http.dump = function (self)
		dump(self, "")
	end

	return true
end

local function parse_response(stream, http)
	local len, total_len

	local line, len = read_line(stream)
	total_len = len

	http.version, http.status, http.reason = line:match("([^%s]+) ([^%s]+) (.+)")
	if not http.version then
		http._invalid = string.format("invalid response '%s'", line)
		return
	end

	total_len = total_len + parse_header(stream, http)

	http.data = stream
	http._length = total_len

	http.dump = function (self)
		dump(self, "")
	end

	return true
end

local function build_headers(stream, headers, headers_order)
	local copy = headers

	for _, name in pairs(headers_order) do
		local value = copy[name]
		if value then
			copy[name] = nil

			stream:insert(name)
			stream:insert(": ")
			stream:insert(value)
			stream:insert("\r\n")
		end
	end

	for name, value in pairs(copy) do
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

		elseif http._state == 5 and not tcp.direction then
			http._state = 0

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

		http._tcp_stream = nil
	end
	return tcp
end

local function parse(http, context, f, name, next_state)
	if not context._co then
		context._mark = http._tcp_stream.stream:mark()
		context._co = coroutine.create(function () f(http._tcp_stream.stream, context) end)
	end

	coroutine.resume(context._co)

	if coroutine.status(context._co) == "dead" then
		if not context._invalid then
			http._state = next_state
			if haka.rule_hook("http-".. name, http) then
				return nil
			end

			context.next_dissector = http.next_dissector
		else
			haka.log.error("http", context._invalid)
			http._tcp_stream:drop()
			return nil
		end
	end
end

haka.dissector {
	name = "http",
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
			http.forge = forge
			http._state = 0

			stream.connection.data._http = http
		end

		local http = stream.connection.data._http
		http._tcp_stream = stream

		if stream.direction then
			if http._state == 0 or http._state == 1 then
				if stream.stream:available() > 0 then
					if http._state == 0 then
						http.request = {}
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
