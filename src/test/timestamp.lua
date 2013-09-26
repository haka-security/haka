
require('protocol/ipv4')
require('protocol/tcp')

haka.rule {
	hooks = { 'ipv4-up' },
	eval = function (self, pkt)
		haka.log("timestamp", "packet timestamp is %f seconds", pkt.raw.timestamp.seconds)
	end
}