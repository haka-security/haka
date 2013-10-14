
local http = require("protocol/http")

http.install_tcp_rule(80)

haka.rule {
	hook = haka.event('http', 'request'),
	eval = function(http, request)
		haka.log.debug("filter", "%s", request.uri)
		if #request.uri > 10 then
			http:reset()
		end
	end
}
