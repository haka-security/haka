
-- Set the path to be able to find the modules in their build folder
package.cpath = package.cpath .. ";" .. string.gsub(arg[4], ':', '/lib?.so;')

local path = string.gsub(arg[4], ':', '/*;')
module.setPath(path);
app.install("packet", module.load("packet-pcap", {"-f", arg[2], "-o", arg[3], "-m"}))

app.install_filter(arg[1])
