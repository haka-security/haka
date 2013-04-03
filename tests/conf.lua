log.message(log.INFO, "config", "Configuring haka...")

app.install("packet", module.load("packet-test"))
app.install("log", module.load("log-stdout"))

app.install_filter(function (pkt)
	log.messagef(log.DEBUG, "filter", "filtering packet [len=%d]", pkt.length)
	return packet.ACCEPT
end)

log.message(log.INFO, "config", "Done\n")

