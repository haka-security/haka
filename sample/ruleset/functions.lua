------------------------------------
-- Function definition
------------------------------------

function getpayload(data)
	local payload = {}
	for i = 1, #data do
		table.insert(payload, string.char(data[i]))
	end
	return table.concat(payload)
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
