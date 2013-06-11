local arg = {...}

-- Set the path to be able to find the modules in their build folder
package.cpath = package.cpath .. ";" .. string.gsub(arg[3], ':', '/?.ho;')

local path = string.gsub(arg[3], ':', '/*;')
haka.module.setpath(path);
haka.app.install("packet", haka.module.load("packet-pcap", "-f", arg[2]))

haka.app.load_configuration(arg[1])
