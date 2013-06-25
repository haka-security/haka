------------------------------------
-- Function definition
------------------------------------

function getpayload(data)
	payload = ''
	for i = 1, #data do
		payload = payload .. string.char(data[i])
	end
	return payload
end

function contains(table, elem)
	return table[elem] ~= nil
end

function dict(table)
	local ret = {}
	for _, v in pairs(table) do
		ret[v] = true
	end
	return ret
end
