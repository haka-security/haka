
function group_by(tab, field)
	local ret = {}
	for _, entry in ipairs(tab) do
		local value = entry[field]
		assert(value, "missing field parameter")
		if not ret[value] then
			ret[value] = {}
		end
		table.insert(ret[value], entry)
	end
	return ret
end

function order_by(tab, order)
	local ret = {}
	for k, v in pairs(tab) do
		table.insert(ret, {k, v})
	end
	table.sort(ret, order)
	return ret
end

function max_length_field(tab)
	local max = {}
	if #tab > 0 then
		for k, v in pairs(tab[1]) do max[k] = k:len() end
	end
	for _, entry in ipairs(tab) do
		for k, v in pairs(entry) do
			local size = v:len()
			if size > max[k] then
				max[k] = size
			end
		end
	end
	return max
end
