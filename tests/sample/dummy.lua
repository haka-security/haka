
require("ipv4")
require("tcp")

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		haka.log.debug("filter", "thread %i: filtering packet [len=%d]", haka.app.currentThread(), pkt.len)
	end
}
