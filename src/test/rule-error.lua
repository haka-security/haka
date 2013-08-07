
local ipv4 = require("protocol/ipv4")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		pkt.none.crash = 1
	end
}
