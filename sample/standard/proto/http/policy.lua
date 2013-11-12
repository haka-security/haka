------------------------------------
-- HTTP Policy
------------------------------------

-- add custom user-agent
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		http.request.headers["User-Agent"] = "Haka User-Agent"
	end
}

-- report and alert if method is different than get and post
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local method = http.request.method:lower()
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
