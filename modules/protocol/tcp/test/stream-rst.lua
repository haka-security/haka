
require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, stream)
		haka.log.debug("filter", "received stream len=%d", stream:available())

		if stream:available() > 10 then
			flow:reset()
		end
	end
}
