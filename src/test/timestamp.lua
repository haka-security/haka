
require('protocol/ipv4')

haka.rule {
	hooks = { 'ipv4-up' },
	eval = function (self, pkt)
		haka.log("timestamp", "packet timestamp is %ds %dus", pkt.raw.timestamp.seconds, pkt.raw.timestamp.micro_seconds)
		haka.log("timestamp", "packet timestamp is %s seconds", pkt.raw.timestamp:toseconds())
		haka.log("timestamp", "packet timestamp is %s", pkt.raw.timestamp)
	end
}
