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

		while pkt.stream:available() > 0 do
			pkt.stream:erase(10)
			local buf = pkt.stream:read(10)
			--if buf then
			--	print(buffer_to_string(buf))
			--end
		end
	end
}

