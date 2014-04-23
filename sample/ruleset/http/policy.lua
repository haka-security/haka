------------------------------------
-- HTTP Policy
------------------------------------

-- add custom user-agent
haka.rule {
	hook = http.events.request,
	eval = function (http, request)
		request.headers["User-Agent"] = "Haka User-Agent"
	end
}

-- report and alert if method is different than get and post
haka.rule {
	hook = http.events.request,
	eval = function (http, request)
		local method = request.method:lower()
		if method ~= 'get' and method ~= 'post' then
			haka.alert{
				description = string.format("forbidden http method '%s'", method),
				sources = haka.alert.address(http.flow.srcip),
				targets = {
					haka.alert.address(http.flow.dstip),
					haka.alert.service(string.format("tcp/%d", http.flow.dstport), "http")
				},
			}
		end
	end
}
