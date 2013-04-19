package.cpath = package.cpath .. ";" .. string.gsub(arg[4], ':', '/?.ho;')

app.install("packet", module.load("packet-pcap", {"-f", arg[2], "-o", arg[3]}))
app.install_filter(dofile(arg[1]))
