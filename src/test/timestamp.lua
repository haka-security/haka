
haka.rule {
	hook = haka.event('raw', 'receive_packet'),
	eval = function (pkt)
		haka.log("timestamp", "packet timestamp is %f seconds", pkt.timestamp.seconds)
	end
}
