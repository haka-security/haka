------------------------------------
-- Function definition
------------------------------------

local function getpayload(data)
	payload = ''
	for i = 1, #data do
		payload = payload .. string.char(data[i])
	end
	return payload
end


local function contains(table, elem)
	return table[elem] ~= nil
end

local function dict(table)
	local ret = {}
	for _, v in pairs(table) do
		ret[v] = true
	end
	return ret
end
