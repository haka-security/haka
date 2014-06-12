-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local module = {}

local class = require('class')
local check = require('check')
local color = require('color')

local list = class.class('list')

local function format_list(headers, formatter, indent, print, content, extra)
	local column_size = {}
	local content_disp = {}
	local sep = ' | '
	indent = indent or ''

	for i,_ in ipairs(content) do
		content_disp[i] = {}
	end
	if extra then content_disp[#content_disp+1] = {} end

	for _,h in ipairs(headers) do
		local max = #h
		local f = formatter and formatter[h] or tostring
		local value

		for i,r in ipairs(content) do
			value = f(r[h])
			content_disp[i][h] = value
			local size = #value
			if size > max then max = size end
		end

		if extra then
			value = f(extra[h])
			content_disp[#content_disp][h] = value
			local size = #value
			if size > max then max = size end
		end

		column_size[h] = max
	end

	local row = {}
	row[1] = ''
	for i,h in ipairs(headers) do
		row[1+3*i-2] = string.format("%s%s%s%s", color.blue, color.bold, h, color.clear)
		row[1+3*i-1] = string.rep(' ', column_size[h]-#h)
		row[1+3*i] = sep
	end
	print(table.concat(row))

	row[1] = indent
	for _,r in ipairs(content_disp) do
		for i,h in ipairs(headers) do
			local value = r[h]
			row[1+3*i-2] = value
			row[1+3*i-1] = string.rep(' ', column_size[h]-#value)
			row[1+3*i] = sep
		end
		print(table.concat(row))
	end
end

function list.method:__init()
	rawset(self, '_data', {})
end

function list.method:__len(self)
	local data = rawget(self, '_data')
	if data then
		return #self._data
	end
end

function list.method:__pprint(indent, print)
	format_list(class.classof(self).field, class.classof(self).field_format,
		indent, print, self._data, self:_aggregate())
end

function list.method:print()
	format_list(class.classof(self).field, class.classof(self).field_format,
		'', print, self._data,  self:_aggregate())
end

function list.method:filter(f)
	check.type(1, f, 'function')

	local newdata = {}
	for _,r in ipairs(rawget(self, '_data') or {}) do
		if f(r) then
			table.insert(newdata, r)
		end
	end
	self._data = newdata
	return self
end

function list.method:sort(order, invert)
	check.types(1, order, {'string', 'function'})

	local data = rawget(self, '_data')
	if data then
		if type(order) == 'function' then
			if invert then
				table.sort(data, function (a, b) return not order(a, b) end)
			else
				table.sort(data, order)
			end
		else
			local found = false

			for _,field in ipairs(class.classof(self).field) do
				if field == order then
					found = true
					break
				end
			end

			if not found then
				check.error(string.format("order '%s' is not a valid key", order))
			end

			if invert then
				table.sort(data, function(a, b) return a[order] > b[order] end)
			else
				table.sort(data, function(a, b) return a[order] < b[order] end)
			end
		end
	end
	return self
end

function list.method:add(data)
	table.append(rawget(self, '_data'), data)
end

function list.method:addall(data)
	for _,d in ipairs(data) do
		self:add(d)
	end
end

function list.method:_aggregate_one(field)
	local aggregator = class.classof(self).field_aggregate
	if aggregator and aggregator[field] then
		local values = {}
		for _,r in ipairs(self._data) do
			values[#values+1] = r[field]
		end
		return aggregator[field](values)
	end
end

function list.method:_aggregate()
	local aggregator = class.classof(self).field_aggregate
	local data = rawget(self, '_data')
	if aggregator and data and #data > 1 then
		local agg = {}
		for _,h in ipairs(class.classof(self).field) do
			agg[h] = self:_aggregate_one(h) or ''
		end
		return agg
	end
end

function list.method:get(name)
	check.types(1, name, {'string', 'number'})

	local data = rawget(self, '_data')
	if data then
		local key = class.classof(self).key
		for _,r in ipairs(data) do
			if r[key] == name then
				local ret = class.classof(self):new()
				ret._data = { r }
				return ret
			end
		end
	end
end

function list.method:__index(name)
	local data = rawget(self, '_data')
	if data then
		if #data == 1 then
			return data[1][name]
		else
			return self:_aggregate_one(name)
		end
	end
end

function module.new(name)
	return class.class(name, list)
end

--
-- Formatter
--

module.formatter = {}

local num_units = { 'k', 'M', 'G', 'T' }

function module.formatter.unit(num)
	if not num or num < 1000 then return tostring(num) end

	for _,u in ipairs(num_units) do
		num = num / 1000
		if num < 1000 then return string.format("%.2f%s", num, u) end
	end

	return string.format("%.2f%s", num, num_units[#num_units])
end

function module.formatter.optional(default)
	return function (val)
		if val then return val
		else return default end
	end
end

--
-- Aggregation
--

module.aggregator = {}

function module.aggregator.add(nums)
	local sum = 0
	for _,n in ipairs(nums) do
		sum = sum + n
	end
	return sum
end

function module.aggregator.replace(value)
	return function (nums) return value end
end

return module
