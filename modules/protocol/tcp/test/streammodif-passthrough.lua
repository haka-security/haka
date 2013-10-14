
require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

local buf = haka.buffer(4)
buf[1] = 0x48
buf[2] = 0x61
buf[3] = 0x6b
buf[4] = 0x61

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, stream)
		stream:insert(buf)
	end
}

haka.rule {
	hook = haka.event('tcp-connection', 'send_data'),
	eval = function (flow, stream)
		stream:erase(10)
	end
}
