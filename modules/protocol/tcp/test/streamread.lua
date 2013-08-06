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

		local buf = pkt.stream:read()
		if buf then
			print(buffer_to_string(buf))
		end
	end
}
