------------------------------------
-- HTTP Policy
------------------------------------

-- add custom user-agent
haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		request.headers["User-Agent"] = "Haka User-Agent"
	end
}

-- report and alert if method is different than get and post
haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		local method = request.method:lower()
		if method ~= 'get' and method ~= 'post' then
			local conn = http.connection
			haka.alert{
				description = string.format("forbidden http method '%s'", method),
				sources = haka.alert.address(conn.srcip),
				targets = {
					haka.alert.address(conn.dstip),
					haka.alert.service(string.format("tcp/%d", conn.dstport), "http")
				},
			}
		end
	end
}
