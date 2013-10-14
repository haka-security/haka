
require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

local function buffer_to_string(buf)
	local r = ""
	for i = 1,#buf do
		r = r .. string.format("%c", buf[i])
	end
	return r
end

haka.rule {
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, stream)
		haka.log.debug("filter", "received stream len=%d", stream:available())

		while stream:available() > 0 do
			stream:erase(10)
			local buf = stream:read(10)
			if buf then
				print(buffer_to_string(buf))
			end
		end
	end
}
