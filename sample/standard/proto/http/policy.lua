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

-- warn if method is different than get and post 
haka.rule {
	hook = haka.event('http', 'request'),
	eval = function (http, request)
		local method = request.method:lower()
		if method ~= 'get' and method ~= 'post' then
			haka.log.warning("filter", "forbidden http method '%s'", method)
		end
	end
}
