print("Configuring haka...")

app.install("packet", module.load("test"))

app.install_filter(function (pkt)
	print("filtering")
	return packet.ACCEPT
end)

print("Done")
