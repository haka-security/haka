-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local function default_cmp(a, b)
	return tostring(a) < tostring(b)
end

function sorted_pairs(t, f)
	f = f or default_cmp
	local a = {}
	for n in pairs(t) do table.insert(a, n) end
	table.sort(a, f)
	local i = 0 -- iterator variable
	local iter = function () -- iterator function
		i = i + 1
		if a[i] == nil then return nil
		else return a[i], t[a[i]]
		end
	end
	return iter
end

-- String extras

function string.safe_format(str, eol)
	local len = #str
	local sstr = {}
	eol = eol or false

	for i=1,len do
		local b = str:byte(i)

		if b >= 0x20 and b <= 0x7e then
			sstr[i] = string.char(b)
		elseif eol and b == 0x0a then
			sstr[i] = '\n'
		else
			sstr[i] = string.format('\\x%.2x', b)
		end
	end

	return table.concat(sstr)
end

-- Table extras

function table.merge(dst, src)
	for k,v in pairs(src) do
		dst[k] = v
	end
end

function table.append(dst, src)
	for _,v in ipairs(src) do
		table.insert(dst, v)
	end
end

function table.copy(src)
	local dst = {}
	for k,v in pairs(src) do
		dst[k] = v
	end
	return dst
end

function table.dict(table)
	local ret = {}
	for _, v in pairs(table) do
		ret[v] = true
	end
	return ret
end

function table.invert(table)
	local ret = {}
	for k, v in pairs(table) do
		ret[v] = k
	end
	return ret
end

function table.contains(table, elem)
	return table[elem] ~= nil
end

if not table.pack then
	function table.pack(...)
		return { n = select('#', ...), ... }
	end
end
