local arg = {...}

haka.app.install("packet", haka.module.load("packet-pcap", "-f", arg[2]))

haka.app.load_configuration(arg[1])
