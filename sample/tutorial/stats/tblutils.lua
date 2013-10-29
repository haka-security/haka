
local module = {}

function new_tab(mt_tab)
	tab = {}
	mt_tab.__index = mt_tab
	setmetatable(tab, mt_tab)
	function mt_tab:select_table(elements, where)
		local ret = {}
		for _, entry in ipairs(self) do
			local selected_entry = {}
			if not where or where(entry) then
				for _, field in ipairs(elements) do
					selected_entry[field] = entry[field]
				end
				table.insert(ret, selected_entry)
			end
		end
		setmetatable(ret, mt_tab)
		return ret
	end

	function mt_tab:top(field, nb)
		nb = nb or 10
		assert(field, "missing field parameter")
		local grouped = group_by(self, field)
		local sorted = order_by(grouped, function(p, q) return #p[2] > #q[2] end)

		for _, entry in ipairs(sorted) do
			if nb <= 0 then break end
			print(entry[1], "(", #entry[2], ")")
			nb = nb - 1
		end
	end

	function mt_tab:dump(nb)
		local max = print_columns(self)
		nb = nb or #self
		local iter = nb
		for _, entry in ipairs(self) do
			if iter <= 0 then break end
			local content = {}
			for k, v in pairs(entry) do
				table.insert(content, string.format("%-" .. tostring(max[k]) .. "s", v))
			end
			print("| " .. table.concat(content, " | ") .. " |")
			iter = iter -1
		end
		if (#self ~= nb) then
			print("... " .. tostring(#self - nb) .. " remaining entries")
		end
	end

	function mt_tab:list()
		if #stats > 1 then
			for k, _ in pairs(self[1]) do
				print(k)
			end
		end
	end

	return tab
end

function group_by(tab, field)
	local ret = {}
	for _, entry in ipairs(tab) do
		local value = entry[field]
		assert(value, "stats are not available on field %s", field)
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

local function max_length_field(tab)
	local max = {}
	for _, entry in ipairs(tab) do
		for k, v in pairs(entry) do
			local size = v:len()
			if size > 99 then
			-- Lua formating limits width to 99
			size = 99
			end
			local cmax = max[k] or 0
			if size > cmax then
				max[k] = size
			end
		end
	end
	return max
end

function print_columns(tab)
	local max = max_length_field(tab)
	if #tab > 0 then
		local columns = {}
		for k, _ in pairs(tab[1]) do
			table.insert(columns, string.format("%-" .. tostring(max[k]) .. "s", k))
		end
	print("| " .. table.concat(columns, " | ") .. " |")
	end
	return max
end

module.new = new_tab
return module
