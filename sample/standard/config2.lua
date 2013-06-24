--Making path correct for calling other lua files
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

--Get path of this file
--mypath =string.format(arg[table.getn(arg)])
mypath ="../../../sample/standard/config.lua"
splitpath=split(mypath,"/")

--local oswd = string.format(os.getenv("PWD"))
oswd="/home/kdenis/Documents/HAKA/haka-runtime/make/out/bin"
local newpath = ""
if #splitpath > 1 then
	for i = 1,#splitpath-1 do
		newpath = newpath .. splitpath[i] .. "/"
	end
	--newpath = os.getenv("PWD") .. "/" .. newpath .. "?.lua;"
	newpath =  oswd .. "/" .. newpath .. "?.lua;"
end
if newpath ~= "" then
	package.path = package.path .. newpath
end

--Loading core rules
require("core/dissectors")
require("core/functions")

-- Loading all rules
require("proto/http/policy")
require("proto/http/rules")
require("proto/http/dissector")
require("proto/http/compliance")
require("proto/http/security")
require("proto/ipv4/policy")
require("proto/ipv4/rules")
require("proto/ipv4/dissector")
require("proto/ipv4/compliance")
require("proto/ipv4/security")
require("proto/tcp/policy")
require("proto/tcp/rules")
require("proto/tcp/policy-connection")
require("proto/tcp/dissector")
require("proto/tcp/security-connection")
require("proto/tcp/compliance")
require("proto/tcp/dissector-connection")
require("proto/tcp/compliance-connection")
require("proto/tcp/security")
require("proto/tcp/rules-connection")
