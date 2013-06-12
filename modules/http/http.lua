module("http", package.seeall)

local function str(char)
	return string.char(char)
end

local function read_line(stream)
	local line = ""
	local char
	local sp = "\r\n"
	
	while true do
		char = stream:getchar()
		if char == -1 then
			return line, false
		end
		char = str(char)

		local tmp = ""
		for i = 1, #sp do
			local c = sp:sub(i,i)
			
			if char ~= c then
				line = line .. tmp
				break
			end

			if i == #sp then
				return line, true
			end

			tmp = tmp .. c
			char = stream:getchar()
			assert(char ~= -1) --TODO: not yet supported
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
	local line = read_line(stream)
	
	http.Method, http.Uri, http.Version = line:match("(%g+) (%g+) (%g+)")
	if not http.Method then
		return false
	end

	-- Headers
	http.Headers = {}
	http.headers_order = {}
	line = read_line(stream)
	while #line > 0 do
		local name, value = line:match("(%g+): (%g+)")
		if not name then
			return false
		end

		http.Headers[name] = value
		table.insert(http.headers_order, name)
		line = read_line(stream)
	end

	http.data = stream

	dump(http, "")

	return true
end

local function parse_response(stream, http)
	local line = read_line(stream)
	
	http.Version, http.Status, http.Reason = line:match("(%g+) (%g+) (%g+)")
	if not http.Version then
		return false
	end

	-- Headers
	http.Headers = {}
	http.headers_order = {}
	line = read_line(stream)
	while #line > 0 do
		local name, value = line:match("(%g+): (%g+)")
		if not name then
			return false
		end

		http.Headers[name] = value
		table.insert(http.headers_order, name)
		line = read_line(stream)
	end

	http.data = stream

	dump(http, "")

	return true
end

local function forge(http)
	if http.state == 1 then
		http.state = 2
	elseif http.state == 3 then
		http.state = 4
	end

	local tcp = http.tcp_stream
	http.tcp_stream = nil
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

		print("HTTP", stream.stream:available(), stream.direction)

		if stream.direction then
			if http.state == 0 then
				if stream.stream:available() > 0 then
					http.request = {}
					parse_request(stream.stream, http.request)
					
					if not haka.rule_hook("http-request", http) then
						return nil
					end
	
					http.request.next_dissector = http.next_dissector
					http.state = 1
				end
			elseif http.state > 0 then
				http.next_dissector = http.request.next_dissector
			end
		else
			if http.state == 2 then
				if stream.stream:available() > 0 then
	
					http.response = {}
					parse_response(stream.stream, http.response)
					
					if not haka.rule_hook("http-response", http) then
						return nil
					end
	
					http.response.next_dissector = http.next_dissector
					http.state = 3
				end
			elseif http.state > 2 then
				http.next_dissector = http.response.next_dissector
			end
		end

		return http
	end
}
