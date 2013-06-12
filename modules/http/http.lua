module("http", package.seeall)

local function str(char)
	return string.char(char)
end

local function read_line(stream)
	local line = ""
	local char
	local sp = "\r\n"
	local read = 0
	
	while true do
		char = stream:getchar()
		if char == -1 then
			return line, read, false
		end
		read = read+1
		char = str(char)

		local tmp = ""
		for i = 1, #sp do
			local c = sp:sub(i,i)
			
			if char ~= c then
				line = line .. tmp
				break
			end

			if i == #sp then
				return line, read, true
			end

			tmp = tmp .. c
			char = stream:getchar()
			assert(char ~= -1) --TODO: not yet supported
			read = read+1
			char = str(char)
		end
		
		line = line .. char
	end
end

local function dump(t, indent)
	for n, v in pairs(t) do
		if type(v) == "table" then
			print(indent, n)
			dump(v, indent .. "  ")
		else
			print(indent, n, "=", v)
		end
	end
end

local function parse_request(stream, http)
	local len, total_len

	total_len = 0

	local line, len = read_line(stream)
	total_len = total_len + len

	http.Method, http.Uri, http.Version = line:match("(%g+) (%g+) (%g+)")
	if not http.Method then
		return false
	end

	-- Headers
	http.Headers = {}
	http.headers_order = {}
	line, len = read_line(stream)
	total_len = total_len + len
	while #line > 0 do
		local name, value = line:match("(%g+): (.+)")
		if not name then
			return false
		end

		http.Headers[name] = value
		table.insert(http.headers_order, name)
		line, len = read_line(stream)
		total_len = total_len + len
	end

	http.data = stream
	http.length = total_len

	dump(http, "")

	return true
end

local function parse_response(stream, http)
	local len, total_len

	total_len = 0

	local line, len = read_line(stream)
	total_len = total_len + len
	
	http.Version, http.Status, http.Reason = line:match("(%g+) (%g+) (%g+)")
	if not http.Version then
		return false
	end

	-- Headers
	http.Headers = {}
	http.headers_order = {}
	line, len = read_line(stream)
	total_len = total_len + len
	while #line > 0 do
		local name, value = line:match("(%g+): (.+)")
		if not name then
			return false
		end

		http.Headers[name] = value
		table.insert(http.headers_order, name)
		line, len = read_line(stream)
		total_len = total_len + len
	end

	http.data = stream
	http.length = total_len

	dump(http, "")

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
	local tcp = http.tcp_stream
	if tcp then
		if http.state == 1 and tcp.direction then
			http.state = 2

			tcp.stream:rewind()
			tcp.stream:erase(http.request.length)
			tcp.stream:insert(http.request.Method)
			tcp.stream:insert(" ")
			tcp.stream:insert(http.request.Uri)
			tcp.stream:insert(" ")
			tcp.stream:insert(http.request.Version)
			tcp.stream:insert("\r\n")
			build_headers(tcp.stream, http.request.Headers, http.request.headers_order)
			tcp.stream:insert("\r\n")

		elseif http.state == 3 and not tcp.direction then
			http.state = 4
			tcp.stream:rewind()
			tcp.stream:erase(http.response.length)
			tcp.stream:insert(http.response.Version)
			tcp.stream:insert(" ")
			tcp.stream:insert(http.response.Status)
			tcp.stream:insert(" ")
			tcp.stream:insert(http.response.Reason)
			tcp.stream:insert("\r\n")
			build_headers(tcp.stream, http.response.Headers, http.response.headers_order)
			tcp.stream:insert("\r\n")
		end
	
		http.tcp_stream = nil
	end
	return tcp
end

haka.dissector {
	name = "http",
	dissect = function (stream)

		if not stream.connection.data.http then
			local http = {}
			http.dissector = "http"
			http.next_dissector = nil
			http.valid = function (self)
				return self.tcp_stream:valid()
			end
			http.drop = function (self)
				return self.tcp_stream:drop()
			end
			http.forge = forge
			http.state = 0
			
			stream.connection.data.http = http
		end
		
		local http = stream.connection.data.http
		http.tcp_stream = stream

		if stream.direction then
			if http.state == 0 then
				if stream.stream:available() > 0 then
					stream.stream:mark()

					http.request = {}
					if not parse_request(stream.stream, http.request) then
						haka.log.error("http", "invalid request")
						stream:drop()
						return nil
					end

					http.state = 1

					if haka.rule_hook("http-request", http) then
						return nil
					end
	
					http.request.next_dissector = http.next_dissector
				end
			elseif http.state > 0 then
				http.next_dissector = http.request.next_dissector
			end
		else
			if http.state == 2 then
				if stream.stream:available() > 0 then
					stream.stream:mark()

					http.response = {}
					if not parse_response(stream.stream, http.response) then
						haka.log.error("http", "invalid response")
						stream:drop()
						return nil
					end

					http.state = 3

					if haka.rule_hook("http-response", http) then
						return nil
					end
	
					http.response.next_dissector = http.next_dissector
				end
			elseif http.state > 2 then
				http.next_dissector = http.response.next_dissector
			end
		end

		return http
	end
}
