
local function group_by(tab, field)
	local ret = {}
	for _, entry in ipairs(tab) do
		local value = entry[field]
		assert(value, string.format("stats are not available on field %s", field))
		if not ret[value] then
			ret[value] = {}
		end
		table.insert(ret[value], entry)
	end
	return ret
end

local function order_by(tab, order)
	local ret = {}
	for k, v in pairs(tab) do
		table.insert(ret, {k, v})
	end
	table.sort(ret, order)
	return ret
end

local function max_length_field(tab, nb)
	local max = {}
	if nb > #tab then nb = #tab end
	for iter = 1, nb do
		for k, v in pairs(tab[iter]) do
			local size = #v
			if size > 99 then
				-- Lua formating limits width to 99
				size = 99
			end
			if not max[k] then
				max[k] = #k
			end
			if size > max[k] then
				max[k] = size
			end
		end
	end
	return max
end

local function print_columns(tab, nb)
	local max = max_length_field(tab, nb)
	if #tab > 0 then
		local columns = {}
		for k, _ in pairs(tab[1]) do
			table.insert(columns, string.format("%-" .. tostring(max[k]) .. "s", k))
		end
		print("| " .. table.concat(columns, " | ") .. " |")
	end
	return max
end

-- Metatable for storing 'sql-like' table and methods.  Intended to be used to
-- store 'stats' data and to provide methods to request them

local table_mt = {}
table_mt.__index = table_mt

function table_mt:select_table(elements, where)
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
	setmetatable(ret, table_mt)
	return ret
end

function table_mt:top(field, nb)
	nb = nb or 10
	assert(field, "missing field parameter")
	local grouped = group_by(self, field)
	local sorted = order_by(grouped, function(p, q) return #p[2] > #q[2] end)

	for _, entry in ipairs(sorted) do
		if nb <= 0 then break end
		print(entry[1], ":", #entry[2])
		nb = nb - 1
	end
end

function table_mt:dump(nb)
	nb = nb or #self
	assert(nb > 0, "illegal negative value")
	local max = print_columns(self, nb)
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
	if #self > nb then
		print("... " .. tostring(#self - nb) .. " remaining entries")
	end
end

function table_mt:list()
	if #stats > 1 then
		for k, _ in pairs(self[1]) do 
			print(k)
		end
	end
end

return {
	new = function ()
		local ret = {}
		setmetatable(ret, table_mt)
		return ret
	end
}
