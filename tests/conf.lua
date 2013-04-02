print("Configuring haka...")

app.install("packet", module.load("test"))

app.install_filter(function (pkt)
	print(string.format("filtering packet [len=%d]", pkt.length))
	return packet.ACCEPT
end)

print("Done")
