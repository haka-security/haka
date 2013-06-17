require("ipv4")
require("tcp")

local function buffer_to_string(buf)
	local r = ""
	for i = 1,#buf do
		r = r .. string.format("%c", buf[i])
	end
	return r
end

haka.rule {
	hooks = { "tcp-connection-up" },
	eval = function (self, pkt)
		haka.log.debug("filter", "received stream len=%d", pkt.stream:available())

		local buf2 = haka.stream.buffer(4)
		buf2[1] = 0x48
		buf2[2] = 0x61
		buf2[3] = 0x6b
		buf2[4] = 0x61

		while pkt.stream:available() > 0 do
			pkt.stream:insert(buf2)
			local buf = pkt.stream:read(10)
			if buf then
				print(buffer_to_string(buf))
			end
		end
	end
}

