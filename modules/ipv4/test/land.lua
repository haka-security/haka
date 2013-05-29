
require('ipv4')

haka2.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if pkt.src == pkt.dst then
			pkt:drop()
		end
	end
}
