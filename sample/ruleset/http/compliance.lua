
------------------------------------
-- Loading regex engine
------------------------------------

local rem = require("regexp/pcre")

------------------------------------
-- HTTP compliance
------------------------------------

-- check http method value
local http_methods = '^get$|^post$|^head$|^put$|^trace$|^delete$|^options$'
local re = rem.re:compile(http_methods, rem.re.CASE_INSENSITIVE)

haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		local method = request.method
		local ret = re:match(method)
		if not ret then
			haka.alert{
				description = string.format("non authorized http method '%s'", method),
				sources = haka.alert.address(http.flow.srcip),
				targets = {
					haka.alert.address(http.flow.dstip),
					haka.alert.service(string.format("tcp/%d", http.flow.dstport), "http")
				},
			}
			http:drop()
		end
	end
}

-- check http version value
local http_versions = '^0.9$|^1.0$|^1.1$'
local re = rem.re:compile(http_versions)
haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		local version = request.version
		local ret = re:match(version)
		if not ret then
			haka.alert{
				description = string.format("unsupported http version '%s'", version),
				sources = haka.alert.address(http.flow.srcip),
				targets = {
					haka.alert.address(http.flow.dstip),
					haka.alert.service(string.format("tcp/%d", http.flow.dstport), "http")
				},
			}
			http:drop()
		end
	end
}

-- check content length value
haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		local content_length = request.headers["Content-Length"] or 0
		content_length = tonumber(content_length)
		if content_length == nil or content_length < 0 then
			haka.alert{
				description = "corrupted content-length header value",
				sources = haka.alert.address(http.flow.srcip),
				targets = {
					haka.alert.address(http.flow.dstip),
					haka.alert.service(string.format("tcp/%d", http.flow.dstport), "http")
				},
			}
			http:drop()
		end
	end
}
