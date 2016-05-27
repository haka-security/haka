------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
local tcp_connection = require('protocol/tcp_connection')
local http = require('protocol/http')

-- Allow only connections on port 80, close all other connections
haka.policy {
	on = haka.dissectors.tcp_connection.policies.new_connection,
	name = "drop by default",
	action = {
		haka.policy.alert{
			description = "drop by default"
		},
		haka.policy.drop
	}
}

haka.policy {
	on = haka.dissectors.tcp_connection.policies.new_connection,
	dstport = 80,
	name = "authorizing http traffic",
	action = haka.policy.accept
}

-- Allow only connections from the 'Mozilla' user agent.
haka.rule{
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		if string.match(request.headers['User-Agent'], 'Mozilla') then
			haka.log("User-Agent Mozilla detected")
		else
			haka.log("Only Mozilla User-Agent authorized")
			http:drop()
		end
	end
}
