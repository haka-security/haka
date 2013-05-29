local arg = {...}

haka.log.info("config", "configuring haka...")

haka.app.install("packet", haka.module.load("packet-pcap", "-f", arg[1]))
haka.app.install("log", haka.module.load("log-stdout"))

haka.app.load_configuration(arg[2])

haka.log.info("config", "done\n")
