local arg = {...}

-- Set the path to be able to find the modules in their build folder
package.cpath = package.cpath .. ";" .. string.gsub(arg[4], ':', '/?.ho;')

local path = string.gsub(arg[4], ':', '/*;')
haka.module.setPath(path);
haka.app.install("packet", haka.module.load("packet-pcap", "-f", arg[2], "-o", arg[3]))

haka.app.load_configuration(arg[1])
