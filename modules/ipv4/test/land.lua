
require('ipv4')

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if pkt.src == pkt.dst then
			pkt:drop()
		end
	end
}
