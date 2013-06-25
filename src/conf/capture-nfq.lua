local arg = {...}

haka.log.info("config", "configuring haka...")

require("utils")

local ifaces = split(arg[1], ":")
haka.app.install("packet", haka.module.load("packet-nfqueue", unpack(ifaces)))

haka.app.install("log", haka.module.load("log-stdout"))

haka.module.addpath(get_file_directory(arg[2]))

haka.app.load_configuration(arg[2])

haka.log.info("config", "done\n")
