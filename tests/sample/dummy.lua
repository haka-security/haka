return function (pkt)
	log.debug("filter", "thread %i: filtering packet [len=%d]", app.currentThread(), pkt.length)
	return packet.ACCEPT
end
