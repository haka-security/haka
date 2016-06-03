------------------------------------
---- Loading regex engine
--------------------------------------

local rem = require("regexp/pcre")


--local http_methods = '^get$|^post$|^head$|^put$|^trace$|^delete$|^options$'
--local re = rem.re:compile(http_methods, rem.re.CASE_INSENSITIVE)
--

------------------------------------
-- HTTP Policy
------------------------------------

-- add custom user-agent
haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		request.headers["User-Agent"] = "Haka User-Agent"
	end
}

-- report and alert if method is different than get and post
haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		local method = request.method
		if not rem.re:match('^get$|^post$', method, rem.re.CASE_INSENSITIVE) then
			haka.alert{
				description = string.format("forbidden http method '%s'", method),
				sources = haka.alert.address(http.srcip),
				targets = {
					haka.alert.address(http.dstip),
					haka.alert.service(string.format("tcp/%d", http.dstport), "http")
				},
			}
		end
	end
}
