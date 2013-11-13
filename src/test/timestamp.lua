
haka.rule {
	hook = haka.event('raw', 'receive_packet'),
	eval = function (pkt)
		haka.log("timestamp", "packet timestamp is %ds %dns", pkt.timestamp.secs, pkt.timestamp.nsecs)
		haka.log("timestamp", "packet timestamp is %s seconds", pkt.timestamp.seconds)
		haka.log("timestamp", "packet timestamp is %s", pkt.timestamp)
	end
}
