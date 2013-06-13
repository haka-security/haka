local arg = {...}

haka.log.info("config", "configuring haka...")

if arg[3] then
	haka.app.install("packet", haka.module.load("packet-pcap", "-f", arg[1], "-o", arg[3]))
else
	haka.app.install("packet", haka.module.load("packet-pcap", "-f", arg[1]))
end

haka.app.install("log", haka.module.load("log-stdout"))

haka.app.load_configuration(arg[2])

haka.log.setlevel(haka.log.DEBUG)
haka.log.setlevel("packet", haka.log.INFO)

haka.log.info("config", "done\n")
