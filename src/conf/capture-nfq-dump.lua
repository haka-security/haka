local arg = {...}

haka.app.install("log", haka.module.load("log/stdout"))

haka.log.info("config", "configuring haka...")

require("utils")

if #arg ~= 2 or arg[1] == "-h" or arg[1] == "--help" then
	print("usage: capture-nfq-dump.lua iface1:iface2:... <configuration>")
	haka.app.exit(0)
end

local ifaces = split(arg[1], ":")
haka.app.install("packet", haka.module.load("packet/nfqueue", "-p", "input.pcap", "output.pcap", "drop.pcap", unpack(ifaces)))

haka.module.addpath(get_file_directory(arg[2]))

haka.app.load_configuration(arg[2])

haka.log.info("config", "done\n")
