-- functions used by different modules
function split(str, delim)
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

function addmodulepath(str)
	splitpath=split(str,"/")
	local newpath = ""
	if #splitpath > 1 then
		for i = 1,#splitpath-1 do
			newpath = newpath .. splitpath[i] .. "/"
		end
	end
	if newpath ~= "" then
		haka.module.setpath(haka.module.path() .. ';' .. newpath .. '*')
	end
end
