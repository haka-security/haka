log.info("config", "Configuring haka...")

app.install("packet", module.load("packet-pcap", {"-i", "any", "-m"}))
app.install("log", module.load("log-stdout"))

app.install_filter(arg[1])

log.info("config", "Done\n")
