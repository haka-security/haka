module("http", package.seeall)

haka.dissector {
	name = "http",
	dissect = function (stream)

		local http = {}
		http.dissector = "http"
		http.next_dissector = nil
		http.valid = function (self)
			return self.tcp_stream:valid()
		end
		http.drop = function (self)
			return self.tcp_stream:drop()
		end
		http.forge = function (self)
			return self.tcp_stream:forge()
		end
		http.tcp_stream = stream
		return http
	end
}
