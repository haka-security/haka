arg = {...}
-- Set the path to be able to find the modules in their build folder
package.cpath = package.cpath .. ";" .. string.gsub(arg[3], ':', '/?.ho;')

local path = string.gsub(arg[3], ':', '/*;')
module.setPath(path);
app.install("packet", module.load("packet-pcap", {"-f", arg[2]}))

app.install_filter(arg[1])
