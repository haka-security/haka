local arg = {...}

haka.log.info("config", "configuring haka...")

local function split(str, delim)
	local ret = {}
	local last_end = 1
	local s, e = str:find(delim, 1)

	while s do
		if s ~= 1 then
			cap = str:sub(last_end, e-1)
			table.insert(ret, cap)
		end

		last_end = e+1
		s, e = str:find(delim, last_end)
	end

	if last_end <= #str then
		cap = str:sub(last_end)
		table.insert(ret, cap)
	end

	return ret
end

local ifaces = split(arg[1], ":")
haka.app.install("packet", haka.module.load("packet-nfqueue", unpack(ifaces)))

haka.app.install("log", haka.module.load("log-stdout"))

haka.app.load_configuration(arg[2])

haka.log.setlevel(haka.log.DEBUG)
haka.log.setlevel("packet", haka.log.INFO)

haka.log.info("config", "done\n")
