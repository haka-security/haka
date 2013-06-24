------------------------------------
-- HTTP compliance
------------------------------------
-- check http method value
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local http_methods = dict({ 'get', 'post', 'head', 'put', 'trace', 'delete', 'options' })
		local method = http.request.method:lower()
		if not contains(http_methods, method) then
			haka.log.error("filter", "non authorized http method '%s'", method)
			http:drop()
		end
	end
}

-- check http version value
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local http_versions = dict({ '0.9', '1.0', '1.1' })
		local protocol = http.request.version:sub(1,4)
		local version = http.request.version:sub(6)
		if not protocol == "HTTP" or not contains(http_versions, version) then
			haka.log.error("filter", "unsupported http version '%s/%s'", protocol, version)
			http:drop()
		end
	end
}

-- check content length value
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local content_length = http.request.headers["Content-Length"]
		if content_length then
			content_length = tonumber(content_length)
			if content_length == nil or content_length < 0 then
				haka.log.error("filter", "corrupted content-length header value")
				http:drop()
			end
		end
	end
}
