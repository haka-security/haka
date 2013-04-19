
app.install("packet", module.load("packet-pcap", {"-f", arg[2]}))

output = io.open(arg[3], "w")
app.install_filter(dofile(arg[1]))
